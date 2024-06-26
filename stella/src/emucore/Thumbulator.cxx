//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Thumbulator.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

//============================================================================
// This class provides Thumb emulation code ("Thumbulator")
//    by David Welch (dwelch@dwelch.com)
// Modified by Fred Quimby
// Code is public domain and used with the author's consent
//============================================================================

#ifdef THUMB_SUPPORT

#include "bspf.hxx"
#include "Base.hxx"
#include "Thumbulator.hxx"
using namespace Common;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::Thumbulator(const uInt16* rom_ptr, uInt16* ram_ptr, bool traponfatal)
  : rom(rom_ptr), ram(ram_ptr) { }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Thumbulator::~Thumbulator() { }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::run( void )
{
  reset();
  for(;;)
  {
    if (execute()) break;
    if (instructions > 500000) // way more than would otherwise be possible
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::fetch16 ( uInt32 addr )
{
  fetches++;

  uInt32 data;
  switch(addr&0xF0000000)
  {
    case 0x00000000: //ROM
      addr &= ROMADDMASK;
      if(addr<0x50)
	return 0;

      addr>>=1;
#ifdef MSB_FIRST
      data=((rom[addr]>>8)|(rom[addr]<<8))&0xffff;
#else
      data=rom[addr];
#endif
      return(data);

    case 0x40000000: //RAM
      addr &= RAMADDMASK;
      addr>>=1;
#ifdef MSB_FIRST
      data=((ram[addr]>>8)|(ram[addr]<<8))&0xffff;
#else
      data=ram[addr];
#endif
      return(data);
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::fetch32 ( uInt32 addr )
{
  uInt32 data;
  switch(addr&0xF0000000)
  {
    case 0x00000000: //ROM
      if(addr<0x50)
      {
        data=read32(addr);
        if(addr==0x00000000) return(data);
        if(addr==0x00000004) return(data);
	return 0;
      }

    case 0x40000000: //RAM
      data =fetch16(addr+2);
      data<<=16;
      data|=fetch16(addr+0);
      return(data);
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write16 ( uInt32 addr, uInt32 data )
{
  if((addr>0x40001fff)&&(addr<0x50000000))
    return;
  else if((addr>0x40000028)&&(addr<0x40000c00))
    return;
  if(addr&1)
    return;

  writes++;

  switch(addr&0xF0000000)
  {
    case 0x40000000: //RAM
      addr&=RAMADDMASK;
      addr>>=1;
#ifdef MSB_FIRST
      ram[addr]=(((data&0xFFFF)>>8)|((data&0xffff)<<8))&0xffff;
#else
      ram[addr]=data&0xFFFF;
#endif
      return;

    case 0xE0000000: //MAMCR
      if(addr == 0xE01FC000)
      {
        mamcr = data;
        return;
      }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::write32 ( uInt32 addr, uInt32 data )
{
  if(addr&3)
    return;

  switch(addr&0xF0000000)
  {
    case 0xF0000000: //halt
#if 1
      return;
#else
      throw "HALT";// exit(0);
#endif

    case 0xE0000000: //periph
      switch(addr)
      {
        case 0xE0000000:
          break;
      }
      return;

    case 0xD0000000: //debug
      return;

    case 0x40000000: //RAM
      write16(addr+0,(data>> 0)&0xFFFF);
      write16(addr+2,(data>>16)&0xFFFF);
      return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read16 ( uInt32 addr )
{
  uInt32 data;

  if((addr>0x40001fff)&&(addr<0x50000000))
    return 0;
  else if((addr>0x7fff)&&(addr<0x10000000))
    return 0;
  if(addr&1)
    return 0;

  reads++;

  switch(addr&0xF0000000)
  {
    case 0x00000000: //ROM
      addr&=ROMADDMASK;
      addr>>=1;
#ifdef MSB_FIRST
      data=((rom[addr]>>8)|(rom[addr]<<8))&0xffff;
#else
      data=rom[addr];
#endif
      return(data);

    case 0x40000000: //RAM
      addr&=RAMADDMASK;
      addr>>=1;
#ifdef MSB_FIRST
      data=((ram[addr]>>8)|(ram[addr]<<8))&0xffff;
#else
      data=ram[addr];
#endif
      return(data);

    case 0xE0000000: //MAMCR
      if(addr == 0xE01FC000)
        return mamcr;
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read32 ( uInt32 addr )
{
  if(addr&3)
    return 0;

  uInt32 data;
  switch(addr&0xF0000000)
  {
    case 0x00000000: //ROM
    case 0x40000000: //RAM
      data =read16(addr+2);
      data<<=16;
      data|=read16(addr+0);
      return(data);
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::read_register ( uInt32 reg )
{
  reg&=0xF;

  uInt32 data;
  switch(cpsr&0x1F)
  {
    case MODE_SVC:
      switch(reg) // TODO (SA) - does this do anything other than default?
      {
        default: data=reg_sys[reg]; break;
        case 13: case 14: data=reg_svc[reg]; break;
      }
      return(data);
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Thumbulator::write_register ( uInt32 reg, uInt32 data )
{
  reg&=0xF;

  switch(cpsr&0x1F)
  {
    case MODE_SVC:
      switch(reg) // TODO (SA) - does this do anything other than default?
      {
        default: reg_sys[reg]=data; break;
        case 13: case 14: reg_svc[reg]=data; break;
      }
      return(data);
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_zflag ( uInt32 x )
{
  if(x==0) cpsr|=CPSR_Z;
  else     cpsr&=~CPSR_Z;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_nflag ( uInt32 x )
{
  if(x&0x80000000) cpsr|=CPSR_N;
  else             cpsr&=~CPSR_N;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_cflag ( uInt32 a, uInt32 b, uInt32 c )
{
  uInt32 rc;

  cpsr&=~CPSR_C;
  rc=(a&0x7FFFFFFF)+(b&0x7FFFFFFF)+c; //carry in
  rc = (rc>>31)+(a>>31)+(b>>31);  //carry out
  if(rc&2)
    cpsr|=CPSR_C;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_sub_vflag ( uInt32 a, uInt32 b, uInt32 c )
{
  cpsr&=~CPSR_V;

  //if the sign bits are different
  if((a&0x80000000)^(b&0x80000000))
  {
    //and result matches b
    if((b&0x80000000)==(c&0x80000000))
      cpsr|=CPSR_V;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_add_vflag ( uInt32 a, uInt32 b, uInt32 c )
{
  cpsr&=~CPSR_V;

  //if sign bits are the same
  if((a&0x80000000)==(b&0x80000000))
  {
    //and the result is different
    if((b&0x80000000)!=(c&0x80000000))
      cpsr|=CPSR_V;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_cflag_bit ( uInt32 x )
{
  if(x) cpsr|=CPSR_C;
  else  cpsr&=~CPSR_C;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Thumbulator::do_vflag_bit ( uInt32 x )
{
  if(x) cpsr|=CPSR_V;
  else  cpsr&=~CPSR_V;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Thumbulator::execute ( void )
{
  uInt32 pc, sp, inst,
         ra,rb,rc,
         rm,rd,rn,rs,
         op;

  pc=read_register(15);
  inst=fetch16(pc-2);
  pc+=2;
  write_register(15,pc);

  instructions++;

  //ADC
  if((inst&0xFFC0)==0x4140)
  {
    rd=(inst>>0)&0x07;
    rm=(inst>>3)&0x07;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra+rb;
    if(cpsr&CPSR_C)
      rc++;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    if(cpsr&CPSR_C) do_cflag(ra,rb,1);
    else            do_cflag(ra,rb,0);
    do_add_vflag(ra,rb,rc);
    return(0);
  }

  //ADD(1) small immediate two registers
  if((inst&0xFE00)==0x1C00)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rb=(inst>>6)&0x7;
    if(rb)
    {
      ra=read_register(rn);
      rc=ra+rb;
      write_register(rd,rc);
      do_nflag(rc);
      do_zflag(rc);
      do_cflag(ra,rb,0);
      do_add_vflag(ra,rb,rc);
      return(0);
    }
    else
    {
      //this is a mov
    }
  }

  //ADD(2) big immediate one register
  if((inst&0xF800)==0x3000)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x7;
    ra=read_register(rd);
    rc=ra+rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,rb,0);
    do_add_vflag(ra,-rb,rc);
    return(0);
  }

  //ADD(3) three registers
  if((inst&0xFE00)==0x1800)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    ra=read_register(rn);
    rb=read_register(rm);
    rc=ra+rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,rb,0);
    do_add_vflag(ra,rb,rc);
    return(0);
  }

  //ADD(4) two registers one or both high no flags
  if((inst&0xFF00)==0x4400)
  {
    if((inst>>6)&3)
    {
      //UNPREDICTABLE
    }
    rd=(inst>>0)&0x7;
    rd|=(inst>>4)&0x8;
    rm=(inst>>3)&0xF;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra+rb;
    write_register(rd,rc);
    return(0);
  }

  //ADD(5) rd = pc plus immediate
  if((inst&0xF800)==0xA000)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x7;
    rb<<=2;
    ra=read_register(15);
    rc=(ra&(~3))+rb;
    write_register(rd,rc);
    return(0);
  }

  //ADD(6) rd = sp plus immediate
  if((inst&0xF800)==0xA800)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x7;
    rb<<=2;
    ra=read_register(13);
    rc=ra+rb;
    write_register(rd,rc);
    return(0);
  }

  //ADD(7) sp plus immediate
  if((inst&0xFF80)==0xB000)
  {
    rb=(inst>>0)&0x7F;
    rb<<=2;
    ra=read_register(13);
    rc=ra+rb;
    write_register(13,rc);
    return(0);
  }

  //AND
  if((inst&0xFFC0)==0x4000)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra&rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //ASR(1) two register immediate
  if((inst&0xF800)==0x1000)
  {
    rd=(inst>>0)&0x07;
    rm=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rc=read_register(rm);
    if(rb==0)
    {
      if(rc&0x80000000)
      {
        do_cflag_bit(1);
        rc=~0;
      }
      else
      {
        do_cflag_bit(0);
        rc=0;
      }
    }
    else
    {
      do_cflag_bit(rc&(1<<(rb-1)));
      ra=rc&0x80000000;
      rc>>=rb;
      if(ra) //asr, sign is shifted in
      {
        rc|=(~0)<<(32-rb);
      }
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //ASR(2) two register
  if((inst&0xFFC0)==0x4100)
  {
    rd=(inst>>0)&0x07;
    rs=(inst>>3)&0x07;
    rc=read_register(rd);
    rb=read_register(rs);
    rb&=0xFF;
    if(rb==0)
    {
    }
    else if(rb<32)
    {
      do_cflag_bit(rc&(1<<(rb-1)));
      ra=rc&0x80000000;
      rc>>=rb;
      if(ra) //asr, sign is shifted in
      {
        rc|=(~0)<<(32-rb);
      }
    }
    else
    {
      if(rc&0x80000000)
      {
        do_cflag_bit(1);
        rc=(~0);
      }
      else
      {
        do_cflag_bit(0);
        rc=0;
      }
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //B(1) conditional branch
  if((inst&0xF000)==0xD000)
  {
    rb=(inst>>0)&0xFF;
    if(rb&0x80)
      rb|=(~0)<<8;
    op=(inst>>8)&0xF;
    rb<<=1;
    rb+=pc;
    rb+=2;
    switch(op)
    {
      case 0x0: //b eq  z set
        if(cpsr&CPSR_Z)
        {
          write_register(15,rb);
        }
        return(0);

      case 0x1: //b ne  z clear
        if(!(cpsr&CPSR_Z))
        {
          write_register(15,rb);
        }
        return(0);

      case 0x2: //b cs c set
        if(cpsr&CPSR_C)
        {
          write_register(15,rb);
        }
        return(0);

      case 0x3: //b cc c clear
        if(!(cpsr&CPSR_C))
        {
          write_register(15,rb);
        }
        return(0);

      case 0x4: //b mi n set
        if(cpsr&CPSR_N)
        {
          write_register(15,rb);
        }
        return(0);

      case 0x5: //b pl n clear
        if(!(cpsr&CPSR_N))
        {
          write_register(15,rb);
        }
        return(0);

      case 0x6: //b vs v set
        if(cpsr&CPSR_V)
        {
          write_register(15,rb);
        }
        return(0);

      case 0x7: //b vc v clear
        if(!(cpsr&CPSR_V))
        {
          write_register(15,rb);
        }
        return(0);

      case 0x8: //b hi c set z clear
        if((cpsr&CPSR_C)&&(!(cpsr&CPSR_Z)))
        {
          write_register(15,rb);
        }
        return(0);

      case 0x9: //b ls c clear or z set
        if((cpsr&CPSR_Z)||(!(cpsr&CPSR_C)))
        {
          write_register(15,rb);
        }
        return(0);

      case 0xA: //b ge N == V
        ra=0;
        if(  (cpsr&CPSR_N) &&  (cpsr&CPSR_V) ) ra++;
        if((!(cpsr&CPSR_N))&&(!(cpsr&CPSR_V))) ra++;
        if(ra)
        {
          write_register(15,rb);
        }
        return(0);

      case 0xB: //b lt N != V
        ra=0;
        if((!(cpsr&CPSR_N))&&(cpsr&CPSR_V)) ra++;
        if((!(cpsr&CPSR_V))&&(cpsr&CPSR_N)) ra++;
        if(ra)
        {
          write_register(15,rb);
        }
        return(0);

      case 0xC: //b gt Z==0 and N == V
        ra=0;
        if(  (cpsr&CPSR_N) &&  (cpsr&CPSR_V) ) ra++;
        if((!(cpsr&CPSR_N))&&(!(cpsr&CPSR_V))) ra++;
        if(cpsr&CPSR_Z) ra=0;
        if(ra)
        {
          write_register(15,rb);
        }
        return(0);

      case 0xD: //b le Z==1 or N != V
        ra=0;
        if((!(cpsr&CPSR_N))&&(cpsr&CPSR_V)) ra++;
        if((!(cpsr&CPSR_V))&&(cpsr&CPSR_N)) ra++;
        if(cpsr&CPSR_Z) ra++;
        if(ra)
        {
          write_register(15,rb);
        }
        return(0);

      case 0xE:
        //undefined instruction
        break;

      case 0xF:
        //swi
        break;
    }
  }

  //B(2) unconditional branch
  if((inst&0xF800)==0xE000)
  {
    rb=(inst>>0)&0x7FF;
    if(rb&(1<<10))
      rb|=(~0)<<11;
    rb<<=1;
    rb+=pc;
    rb+=2;
    write_register(15,rb);
    return(0);
  }

  //BIC
  if((inst&0xFFC0)==0x4380)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra&(~rb);
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //BKPT
  if((inst&0xFF00)==0xBE00)
  {
    rb=(inst>>0)&0xFF;
    return(1);
  }

  //BL/BLX(1)
  if((inst&0xE000)==0xE000) //BL,BLX
  {
    if((inst&0x1800)==0x1000) //H=b10
    {
      halfadd=inst;
      return(0);
    }
    else if((inst&0x1800)==0x1800) //H=b11
    {
      //branch to thumb
      rb=halfadd&((1<<11)-1);
      if(rb&1<<10)
        rb|=(~((1<<11)-1)); //sign extend
      rb<<=11;
      rb|=inst&((1<<11)-1);
      rb<<=1;
      rb+=pc;
      write_register(14,pc-2);
      write_register(15,rb);
      return(0);
    }
    else if((inst&0x1800)==0x0800) //H=b01
    {
      // fxq: this should exit the code without having to detect it
      return(1);
    }
  }

  //BLX(2)
  if((inst&0xFF87)==0x4780)
  {
    rm=(inst>>3)&0xF;
    rc=read_register(rm);
    rc+=2;
    if(rc&1)
    {
      write_register(14,pc-2);
      write_register(15,rc);
      return(0);
    }
    else
    {
      // fxq: this could serve as exit code
      return(1);
    }
  }

  //BX
  if((inst&0xFF87)==0x4700)
  {
    rm=(inst>>3)&0xF;
    rc=read_register(rm);
    rc+=2;
    if(rc&1)
    {
      write_register(15,rc);
      return(0);
    }
    else
    {
      // fxq: or maybe this one??
      return(1);
    }
  }

  //CMN
  if((inst&0xFFC0)==0x42C0)
  {
    rn=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rn);
    rb=read_register(rm);
    rc=ra+rb;
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,rb,0);
    do_add_vflag(ra,rb,rc);
    return(0);
  }

  //CMP(1) compare immediate
  if((inst&0xF800)==0x2800)
  {
    rb=(inst>>0)&0xFF;
    rn=(inst>>8)&0x07;
    ra=read_register(rn);
    rc=ra-rb;
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,~rb,1);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //CMP(2) compare register
  if((inst&0xFFC0)==0x4280)
  {
    rn=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rn);
    rb=read_register(rm);
    rc=ra-rb;
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,~rb,1);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //CMP(3) compare high register
  if((inst&0xFF00)==0x4500)
  {
    if(((inst>>6)&3)==0x0)
    {
      //UNPREDICTABLE
    }
    rn=(inst>>0)&0x7;
    rn|=(inst>>4)&0x8;
    if(rn==0xF)
    {
      //UNPREDICTABLE
    }
    rm=(inst>>3)&0xF;
    ra=read_register(rn);
    rb=read_register(rm);
    rc=ra-rb;
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,~rb,1);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //CPS
  if((inst&0xFFE8)==0xB660)
    return(1);

  //CPY copy high register
  if((inst&0xFFC0)==0x4600)
  {
    //same as mov except you can use both low registers
    //going to let mov handle high registers
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    rc=read_register(rm);
    write_register(rd,rc);
    return(0);
  }

  //EOR
  if((inst&0xFFC0)==0x4040)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra^rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //LDMIA
  if((inst&0xF800)==0xC800)
  {
    rn=(inst>>8)&0x7;
    sp=read_register(rn);
    for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
    {
      if(inst&rb)
      {
        write_register(ra,read32(sp));
        sp+=4;
      }
    }
    write_register(rn,sp);
    return(0);
  }

  //LDR(1) two register immediate
  if((inst&0xF800)==0x6800)
  {
    rd=(inst>>0)&0x07;
    rn=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rb<<=2;
    rb=read_register(rn)+rb;
    rc=read32(rb);
    write_register(rd,rc);
    return(0);
  }

  //LDR(2) three register
  if((inst&0xFE00)==0x5800)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read32(rb);
    write_register(rd,rc);
    return(0);
  }

  //LDR(3)
  if((inst&0xF800)==0x4800)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x07;
    rb<<=2;
    ra=read_register(15);
    ra&=~3;
    rb+=ra;
    rc=read32(rb);
    write_register(rd,rc);
    return(0);
  }

  //LDR(4)
  if((inst&0xF800)==0x9800)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x07;
    rb<<=2;
    ra=read_register(13);
    //ra&=~3;
    rb+=ra;
    rc=read32(rb);
    write_register(rd,rc);
    return(0);
  }

  //LDRB(1)
  if((inst&0xF800)==0x7800)
  {
    rd=(inst>>0)&0x07;
    rn=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rb=read_register(rn)+rb;
    rc=read16(rb&(~1));
    if(rb&1)
    {
      rc>>=8;
    }
    else
    {
    }
    write_register(rd,rc&0xFF);
    return(0);
  }

  //LDRB(2)
  if((inst&0xFE00)==0x5C00)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read16(rb&(~1));
    if(rb&1)
    {
      rc>>=8;
    }
    else
    {
    }
    write_register(rd,rc&0xFF);
    return(0);
  }

  //LDRH(1)
  if((inst&0xF800)==0x8800)
  {
    rd=(inst>>0)&0x07;
    rn=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rb<<=1;
    rb=read_register(rn)+rb;
    rc=read16(rb);
    write_register(rd,rc&0xFFFF);
    return(0);
  }

  //LDRH(2)
  if((inst&0xFE00)==0x5A00)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read16(rb);
    write_register(rd,rc&0xFFFF);
    return(0);
  }

  //LDRSB
  if((inst&0xFE00)==0x5600)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read16(rb&(~1));
    if(rb&1)
    {
      rc>>=8;
    }
    else
    {
    }
    rc&=0xFF;
    if(rc&0x80) rc|=((~0)<<8);
    write_register(rd,rc);
    return(0);
  }

  //LDRSH
  if((inst&0xFE00)==0x5E00)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read16(rb);
    rc&=0xFFFF;
    if(rc&0x8000) rc|=((~0)<<16);
    write_register(rd,rc);
    return(0);
  }

  //LSL(1)
  if((inst&0xF800)==0x0000)
  {
    rd=(inst>>0)&0x07;
    rm=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rc=read_register(rm);
    if(rb==0)
    {
      //if immed_5 == 0
      //C unnaffected
      //result not shifted
    }
    else
    {
      //else immed_5 > 0
      do_cflag_bit(rc&(1<<(32-rb)));
      rc<<=rb;
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //LSL(2) two register
  if((inst&0xFFC0)==0x4080)
  {
    rd=(inst>>0)&0x07;
    rs=(inst>>3)&0x07;
    rc=read_register(rd);
    rb=read_register(rs);
    rb&=0xFF;
    if(rb==0)
    {
    }
    else if(rb<32)
    {
      do_cflag_bit(rc&(1<<(32-rb)));
      rc<<=rb;
    }
    else if(rb==32)
    {
      do_cflag_bit(rc&1);
      rc=0;
    }
    else
    {
      do_cflag_bit(0);
      rc=0;
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //LSR(1) two register immediate
  if((inst&0xF800)==0x0800)
  {
    rd=(inst>>0)&0x07;
    rm=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rc=read_register(rm);
    if(rb==0)
    {
      do_cflag_bit(rc&0x80000000);
      rc=0;
    }
    else
    {
      do_cflag_bit(rc&(1<<(rb-1)));
      rc>>=rb;
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //LSR(2) two register
  if((inst&0xFFC0)==0x40C0)
  {
    rd=(inst>>0)&0x07;
    rs=(inst>>3)&0x07;
    rc=read_register(rd);
    rb=read_register(rs);
    rb&=0xFF;
    if(rb==0)
    {
    }
    else if(rb<32)
    {
      do_cflag_bit(rc&(1<<(32-rb)));
      rc>>=rb;
    }
    else if(rb==32)
    {
      do_cflag_bit(rc&0x80000000);
      rc=0;
    }
    else
    {
      do_cflag_bit(0);
      rc=0;
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //MOV(1) immediate
  if((inst&0xF800)==0x2000)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x07;
    write_register(rd,rb);
    do_nflag(rb);
    do_zflag(rb);
    return(0);
  }

  //MOV(2) two low registers
  if((inst&0xFFC0)==0x1C00)
  {
    rd=(inst>>0)&7;
    rn=(inst>>3)&7;
    rc=read_register(rn);
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag_bit(0);
    do_vflag_bit(0);
    return(0);
  }

  //MOV(3)
  if((inst&0xFF00)==0x4600)
  {
    rd=(inst>>0)&0x7;
    rd|=(inst>>4)&0x8;
    rm=(inst>>3)&0xF;
    rc=read_register(rm);
    if (rd==15) rc+=2; // fxq fix for MOV R15
    write_register(rd,rc);
    return(0);
  }

  //MUL
  if((inst&0xFFC0)==0x4340)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra*rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //MVN
  if((inst&0xFFC0)==0x43C0)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rm);
    rc=(~ra);
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //NEG
  if((inst&0xFFC0)==0x4240)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rm);
    rc=0-ra;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(0,~ra,1);
    do_sub_vflag(0,ra,rc);
    return(0);
  }

  //ORR
  if((inst&0xFFC0)==0x4300)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra|rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //POP
  if((inst&0xFE00)==0xBC00)
  {

    sp=read_register(13);
    for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
    {
      if(inst&rb)
      {
        write_register(ra,read32(sp));
        sp+=4;
      }
    }
    if(inst&0x100)
    {
      rc=read32(sp);
      rc+=2;
      write_register(15,rc);
      sp+=4;
    }
    write_register(13,sp);
    return(0);
  }

  //PUSH
  if((inst&0xFE00)==0xB400)
  {

    sp=read_register(13);
    for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
    {
      if(inst&rb)
      {
        rc++;
      }
    }
    if(inst&0x100) rc++;
    rc<<=2;
    sp-=rc;
    rd=sp;
    for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
    {
      if(inst&rb)
      {
        write32(rd,read_register(ra));
        rd+=4;
      }
    }
    if(inst&0x100)
    {
      write32(rd,read_register(14));
    }
    write_register(13,sp);
    return(0);
  }

  //REV
  if((inst&0xFFC0)==0xBA00)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    ra=read_register(rn);
    rc =((ra>> 0)&0xFF)<<24;
    rc|=((ra>> 8)&0xFF)<<16;
    rc|=((ra>>16)&0xFF)<< 8;
    rc|=((ra>>24)&0xFF)<< 0;
    write_register(rd,rc);
    return(0);
  }

  //REV16
  if((inst&0xFFC0)==0xBA40)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    ra=read_register(rn);
    rc =((ra>> 0)&0xFF)<< 8;
    rc|=((ra>> 8)&0xFF)<< 0;
    rc|=((ra>>16)&0xFF)<<24;
    rc|=((ra>>24)&0xFF)<<16;
    write_register(rd,rc);
    return(0);
  }

  //REVSH
  if((inst&0xFFC0)==0xBAC0)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    ra=read_register(rn);
    rc =((ra>> 0)&0xFF)<< 8;
    rc|=((ra>> 8)&0xFF)<< 0;
    if(rc&0x8000) rc|=0xFFFF0000;
    else          rc&=0x0000FFFF;
    write_register(rd,rc);
    return(0);
  }

  //ROR
  if((inst&0xFFC0)==0x41C0)
  {
    rd=(inst>>0)&0x7;
    rs=(inst>>3)&0x7;
    rc=read_register(rd);
    ra=read_register(rs);
    ra&=0xFF;
    if(ra==0)
    {
    }
    else
    {
      ra&=0x1F;
      if(ra==0)
      {
        do_cflag_bit(rc&0x80000000);
      }
      else
      {
        do_cflag_bit(rc&(1<<(ra-1)));
        rb=rc<<(32-ra);
        rc>>=ra;
        rc|=rb;
      }
    }
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //SBC
  if((inst&0xFFC0)==0x4180)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rd);
    rb=read_register(rm);
    rc=ra-rb;
    if(!(cpsr&CPSR_C)) rc--;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,rb,0);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //SETEND
  if((inst&0xFFF7)==0xB650)
    return(1);

  //STMIA
  if((inst&0xF800)==0xC000)
  {
    rn=(inst>>8)&0x7;

    sp=read_register(rn);
    for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
    {
      if(inst&rb)
      {
        write32(sp,read_register(ra));
        sp+=4;
      }
    }
    write_register(rn,sp);
    return(0);
  }

  //STR(1)
  if((inst&0xF800)==0x6000)
  {
    rd=(inst>>0)&0x07;
    rn=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rb<<=2;
    rb=read_register(rn)+rb;
    rc=read_register(rd);
    write32(rb,rc);
    return(0);
  }

  //STR(2)
  if((inst&0xFE00)==0x5000)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read_register(rd);
    write32(rb,rc);
    return(0);
  }

  //STR(3)
  if((inst&0xF800)==0x9000)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x07;
    rb<<=2;
    rb=read_register(13)+rb;
    rc=read_register(rd);
    write32(rb,rc);
    return(0);
  }

  //STRB(1)
  if((inst&0xF800)==0x7000)
  {
    rd=(inst>>0)&0x07;
    rn=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rb=read_register(rn)+rb;
    rc=read_register(rd);
    ra=read16(rb&(~1));
    if(rb&1)
    {
      ra&=0x00FF;
      ra|=rc<<8;
    }
    else
    {
      ra&=0xFF00;
      ra|=rc&0x00FF;
    }
    write16(rb&(~1),ra&0xFFFF);
    return(0);
  }

  //STRB(2)
  if((inst&0xFE00)==0x5400)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read_register(rd);
    ra=read16(rb&(~1));
    if(rb&1)
    {
      ra&=0x00FF;
      ra|=rc<<8;
    }
    else
    {
      ra&=0xFF00;
      ra|=rc&0x00FF;
    }
    write16(rb&(~1),ra&0xFFFF);
    return(0);
  }

  //STRH(1)
  if((inst&0xF800)==0x8000)
  {
    rd=(inst>>0)&0x07;
    rn=(inst>>3)&0x07;
    rb=(inst>>6)&0x1F;
    rb<<=1;
    rb=read_register(rn)+rb;
    rc=read_register(rd);
    write16(rb,rc&0xFFFF);
    return(0);
  }

  //STRH(2)
  if((inst&0xFE00)==0x5200)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    rb=read_register(rn)+read_register(rm);
    rc=read_register(rd);
    write16(rb,rc&0xFFFF);
    return(0);
  }

  //SUB(1)
  if((inst&0xFE00)==0x1E00)
  {
    rd=(inst>>0)&7;
    rn=(inst>>3)&7;
    rb=(inst>>6)&7;
    ra=read_register(rn);
    rc=ra-rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,~rb,1);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //SUB(2)
  if((inst&0xF800)==0x3800)
  {
    rb=(inst>>0)&0xFF;
    rd=(inst>>8)&0x07;
    ra=read_register(rd);
    rc=ra-rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,~rb,1);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //SUB(3)
  if((inst&0xFE00)==0x1A00)
  {
    rd=(inst>>0)&0x7;
    rn=(inst>>3)&0x7;
    rm=(inst>>6)&0x7;
    ra=read_register(rn);
    rb=read_register(rm);
    rc=ra-rb;
    write_register(rd,rc);
    do_nflag(rc);
    do_zflag(rc);
    do_cflag(ra,~rb,1);
    do_sub_vflag(ra,rb,rc);
    return(0);
  }

  //SUB(4)
  if((inst&0xFF80)==0xB080)
  {
    rb=inst&0x7F;
    rb<<=2;
    ra=read_register(13);
    ra-=rb;
    write_register(13,ra);
    return(0);
  }

  //SWI
  if((inst&0xFF00)==0xDF00)
  {
    rb=inst&0xFF;
    return(1);
  }

  //SXTB
  if((inst&0xFFC0)==0xB240)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rm);
    rc=ra&0xFF;
    if(rc&0x80) rc|=(~0)<<8;
    write_register(rd,rc);
    return(0);
  }

  //SXTH
  if((inst&0xFFC0)==0xB200)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rm);
    rc=ra&0xFFFF;
    if(rc&0x8000) rc|=(~0)<<16;
    write_register(rd,rc);
    return(0);
  }

  //TST
  if((inst&0xFFC0)==0x4200)
  {
    rn=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rn);
    rb=read_register(rm);
    rc=ra&rb;
    do_nflag(rc);
    do_zflag(rc);
    return(0);
  }

  //UXTB
  if((inst&0xFFC0)==0xB2C0)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rm);
    rc=ra&0xFF;
    write_register(rd,rc);
    return(0);
  }

  //UXTH
  if((inst&0xFFC0)==0xB280)
  {
    rd=(inst>>0)&0x7;
    rm=(inst>>3)&0x7;
    ra=read_register(rm);
    rc=ra&0xFFFF;
    write_register(rd,rc);
    return(0);
  }

  return(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Thumbulator::reset ( void )
{
  //memset(ram,0xFF,sizeof(ram));
  cpsr=CPSR_T|CPSR_I|CPSR_F|MODE_SVC;

  reg_svc[13]=0x40001fb4; //sp
  reg_svc[14]=0x00000c00; //lr (duz this use odd addrs)
	reg_sys[15]=0x00000c0b; // entry point of 0xc09+2
  //  reg_sys[15]+=2;
  mamcr = 0;

  // fxq: don't care about below so much (maybe to guess timing???)
  instructions=0;
  fetches=0;
  reads=0;
  writes=0;

  return(0);
}

#endif

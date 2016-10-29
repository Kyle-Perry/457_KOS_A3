/******************************************************************************
    Copyright © 2012-2015 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "runtime/Thread.h"
#include "kernel/AddressSpace.h"
#include "kernel/Clock.h"
#include "kernel/Output.h"
#include "world/Access.h"
#include "machine/Machine.h"
#include "devices/Keyboard.h"

#include "main/UserMain.h"

AddressSpace kernelSpace(true); // AddressSpace.h
volatile mword Clock::tick;     // Clock.h

extern Keyboard keyboard;

#if TESTING_KEYCODE_LOOP
static void keybLoop() {
  for (;;) {
    Keyboard::KeyCode c = keyboard.read();
    StdErr.print(' ', FmtHex(c));
  }
}
#endif

void kosMain() {

  int minGranularity = 0;
  int epochLen = 0;
  int parsed = 0;
  bool firstRead = false;
  Scheduler* target = NULL;

  KOUT::outl("Welcome to KOS!", kendl);
  auto iter = kernelFS.find("motb");
  if (iter == kernelFS.end()) {
    KOUT::outl("motb information not found");
  } else {
    FileAccess f(iter->second);
    for (;;) {
      char c;
      if (f.read(&c, 1) == 0) break;
      KOUT::out1(c);
    }
    KOUT::outl();
  }

//find and open the schedparam fo;e
  iter = kernelFS.find("schedparam");
  if (iter == kernelFS.end()) {
    KOUT::outl("schedparam information not found");
  } else {
    FileAccess f(iter->second);
    for (;;) {	
      char c;
      if (f.read(&c, 1) == 0) break;
      KOUT::out1(c);
	//if the character from schedparam is a number character:
	//move the current value of parsed one decimal place to the left, then add the number to it.      
	if( (c >= '0') && (c <= '9'))
	parsed = (parsed * 10) + (c - '0');
	//if the character read is not a number, but a number has been previously read:
	//assign the value to minGranularity if it is the first read, otherwise assign it to epochLen. 
	//Afterward reset the value of parsed to 0 to prepare for the next number to be read.       
	else if ( parsed != 0 )
	{
	  if (!firstRead)
	    {
	      minGranularity = parsed;
	      firstRead = !firstRead;	      
	    }
	  else
	    {
	      epochLen = parsed;
	    }
	  parsed = 0;
	}

    }

	KOUT::outl("Determining ticks per second...");
	mword a = CPU::readTSC();
	Timeout::sleep(1000);
	mword b = CPU::readTSC();
	KOUT::outl(a);
	KOUT::outl(b);
	KOUT::outl(b-a);
    KOUT::outl();
	
	KOUT::outl("Converting scheduler parameters from milliseconds to TSC ticks...");
	minGranularity = minGranularity * (b/1000);
	epochLen = epochLen * (b/1000);
	KOUT::out1("Updated value of minGranularity = ");
	KOUT::outl(minGranularity);
	KOUT::out1("Updated value of epochLen = ");
	KOUT::outl(epochLen);
	KOUT::outl();
	
	KOUT::outl("Updating scheduler parameters to new minGranularity and epochLen...");
	
	mword count = Machine::getProcessorCount();	
	for(mword i = 0; i < count; i++)
	{
		target = Machine::getScheduler(i);
		if(target == NULL)
			break;
		target->setMinGranularity(minGranularity);
		target->setEpochLen(epochLen);
		KOUT::outl("Update success!");
	}
	
	KOUT::outl();
  }
#if TESTING_TIMER_TEST
  StdErr.print(" timer test, 3 secs...");
  for (int i = 0; i < 3; i++) {
    Timeout::sleep(Clock::now() + 1000);
    StdErr.print(' ', i+1);
  }
  StdErr.print(" done.", kendl);
#endif
#if TESTING_KEYCODE_LOOP
  Thread* t = Thread::create()->setPriority(topPriority);
  Machine::setAffinity(*t, 0);
  t->start((ptr_t)keybLoop);
#endif
  Thread::create()->start((ptr_t)UserMain);
#if TESTING_PING_LOOP
  for (;;) {
    Timeout::sleep(Clock::now() + 1000);
    KOUT::outl("...ping...");
  }
#endif
}

extern "C" void kmain(mword magic, mword addr, mword idx)         __section(".boot.text");
extern "C" void kmain(mword magic, mword addr, mword idx) {
  if (magic == 0 && addr == 0xE85250D6) {
    // low-level machine-dependent initialization on AP
    Machine::initAP(idx);
  } else {
    // low-level machine-dependent initialization on BSP -> starts kosMain
    Machine::initBSP(magic, addr, idx);
  }
}

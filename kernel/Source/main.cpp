//========= Copyright N11 Software, All rights reserved. ============//
//
// File: main.cpp
// Purpose: Main point of the kernel.
// Maintainer: aristonl, FiReLScar
//
//===================================================================//

#include <unistd.h>
#include <stdint.h>

#include <BOB.hpp>
#include <COM.hpp>
#include <Config.hpp>
#include <GDT.hpp>
#include <sys/io.h>
#include <Interrupts/APIC.hpp>
#include <Interrupts/DoublePageFault.hpp>
#include <Interrupts/Interrupts.hpp>
#include <Interrupts/PageFault.hpp>
#include <Interrupts/Syscall.hpp>
#include <Memory/Memory.hpp>
#include <Memory/Mem_.hpp>
#include <CPU/CPUID.hpp>
#include <log.h>
#include <TTY.hpp>

// Drivers
#include <drivers/rtc/rtc.h>
#include <drivers/pci/pci.h>

extern unsigned long long InfernoStart;
extern unsigned long long InfernoEnd;

__attribute__((sysv_abi)) void Inferno(BOB* bob) {
	prInfo("kernel", "Inferno kernel version 0.1alpha");
	kprintf("Copyright 2021-2023 N11 Software.\nCopyright 2018-2021 Ariston Lorenzo and Levi Hicks. All rights reserved.\nSee COPYRIGHT in the Inferno source tree for more information.\n");

	// Memory
	Memory::Init(bob);
	prInfo("mm", "Total Memory: %dMB", Memory::GetSize()/0x1000/1024);

	// Create GDT
	#if EnableGDT == true
		GDT::Table GDT = {
			{ 0, 0, 0, 0x00, 0x00, 0 },
			{ 0, 0, 0, 0x9a, 0xa0, 0 },
			{ 0, 0, 0, 0x92, 0xa0, 0 },
			{ 0, 0, 0, 0xfa, 0xa0, 0 },
			{ 0, 0, 0, 0xf2, 0xa0, 0 },
		};
		GDT::Descriptor descriptor;
		descriptor.size = sizeof(GDT) - 1;
		descriptor.offset = (unsigned long long)&GDT;
		LoadGDT(&descriptor);
		prInfo("kernel", "initalized GDT");
	#endif

    // Create IDT
    Interrupts::CreateIDT();

    // Create a test ISR
    Interrupts::CreateISR(0x80, (void*)SyscallHandler);
    Interrupts::CreateISR(0x0E, (void*)PageFault);
    Interrupts::CreateISR(0x08, (void*)DoublePageFault);

    // Load APIC
    if (APIC::Capable()) {
        APIC::Enable();
        prInfo("apic", "initalized APIC");
    }

    // Load IDT
    Interrupts::LoadIDT();
    prInfo("idt", "initialized IDT");

    unsigned long res = 0;
    asm volatile("int $0x80" : "=a"(res) : "a"(1), 
			"d"((unsigned long)"Hello from syscall\n\r"), 
			"D"((unsigned long)0) : "rcx", "r11", "memory");

	RTC::init();

	PCI::init();
	
}

__attribute__((ms_abi)) [[noreturn]] void main(BOB* bob) {
	SetFramebuffer(bob->framebuffer);
	SetFont(bob->FontFile);
	test();
    Inferno(bob);

    // Once finished say hello and halt
    prInfo("kernel", "Done!");

    while (true) asm("hlt");
}

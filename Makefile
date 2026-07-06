NEWLIB_INC = /home/Kittyy/newlib/x86_64-elf/include
CFLAGS = -ffreestanding -m64 -mno-red-zone -fno-stack-protector -O1 -Isrc/include
NASMFLAGS = -f bin

prepare:
	@mkdir -p build/iso
	@mkdir -p build/kernel/object
	@mkdir -p build/bootloader/object

bootloader:
	@echo "Compiling bootloader..."
	@nasm -f bin src/bootloader/stage1.asm -o build/bootloader/object/stage1.bin
	@nasm -f bin src/bootloader/stage2.asm -o build/bootloader/object/stage2.bin

	@echo "Writing Bootloader into Image..."

	@sudo dd if=stage1.bin of=build/fat.img bs=512 seek=0 conv=notrunc
	@sudo dd if=stage2.bin of=build/fat.img bs=512 seek=3 conv=notrunc


kernel:
	@echo "Compiling header files..."

	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/header/stdlib.c -o build/kernel/object/stdlib.o # New 
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/header/string.c -o build/kernel/object/string.o # New

	@echo "Compiling assembly files..."

	@nasm $(NASMFLAGS) src/kernel/start/loader.asm -o build/stub.bin -l build/stub.lst
	@nasm -f elf64 src/ProcessManager/context_switch.asm -o build/kernel/object/context_switch.o
	@nasm -f elf64 src/kernel/start/start.asm -o build/kernel/object/start.o
	@nasm -f elf64 src/kernel/arch/x86_64/idt/isr.asm -o build/kernel/object/isr_stub.o
	@nasm -f elf64 src/kernel/arch/x86_64/gdt/gdt.asm -o build/kernel/object/gdt_load.o
	
	@echo "Compiling C files..."

	@x86_64-elf-gcc $(CFLAGS) -c src/ProcessManager/process.c -o build/kernel/object/process.o # New
	@x86_64-elf-gcc $(CFLAGS) -c src/filesystem/fat.c -o build/kernel/object/fat.o 
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/syscall/heap.c -o build/kernel/object/heap.o                                                                                                                                                                                                                                                                                                          
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/vga/vga.c -o build/kernel/object/vga.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/keyboard/keyboard.c -o build/kernel/object/keyboard.o 
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/syscall/syscall_handler.c -o build/kernel/object/syscall_handler.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/ports.c -o build/kernel/object/ports.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/idt/isr.c -o build/kernel/object/isr.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/idt/idt.c -o build/kernel/object/idt.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/kernel.c -o build/kernel/object/kernel.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/panic/panic.c -o build/kernel/object/panic.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/gdt/gdt.c -o build/kernel/object/gdt.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/shell/shell.c -o build/kernel/object/shell.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/mm/mmu.c -o build/kernel/object/mmu.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/mm/alloc.c -o build/kernel/object/alloc.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/bios/bios.c -o build/kernel/object/bios.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/ata/ata.c -o build/kernel/object/ata_dma.o
	@x86_64-elf-gcc $(CFLAGS) -c src/filesystem/cd.c -o build/kernel/object/cd.o
	@x86_64-elf-gcc $(CFLAGS) -c src/filesystem/disk.c -o build/kernel/object/disk.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/ahci/ahci.c -o build/kernel/object/ahci.o
	@x86_64-elf-gcc $(CFLAGS) -c src/filesystem/usb.c -o build/kernel/object/usb.o
	@x86_64-elf-gcc $(CFLAGS) -c src/filesystem/storage_init.c -o build/kernel/object/storage_init.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/xhci/xhci.c -o build/kernel/object/xhci.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/pci.c -o build/kernel/object/pci.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/xhci/handler.c -o build/kernel/object/handler.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/pmm/pmm.c -o build/kernel/object/pmm.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/dma/dma.c -o build/kernel/object/dma.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/bios/e820/e820.c -o build/kernel/object/e820.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/tsc/tsc.c -o build/kernel/object/tsc.o

	@echo "Compiling Kernel..."

	@x86_64-elf-ld -m elf_x86_64 \
	  -T linker.ld \
	  build/kernel/irq1_handler.o \
      build/kernel/object/start.o \
	  build/kernel/object/kernel.o \
	  build/kernel/object/isr_stub.o \
      build/kernel/object/idt.o \
      build/kernel/object/vga.o \
	  build/kernel/object/mmu.o \
	  build/kernel/object/disk.o \
	  build/kernel/object/handler.o \
	  build/kernel/object/pci.o \
	  build/kernel/object/usb.o \
	  build/kernel/object/xhci.o \
	  build/kernel/object/storage_init.o \
	  build/kernel/object/alloc.o \
      build/kernel/object/keyboard.o \
	  build/kernel/object/shell.o \
	  build/kernel/object/ahci.o \
	  build/kernel/object/cd.o \
	  build/kernel/object/ata_dma.o \
	  build/kernel/object/bios.o \
      build/kernel/object/syscall_handler.o \
	  build/kernel/object/gdt.o \
	  build/kernel/object/gdt_load.o \
      build/kernel/object/ports.o \
      build/kernel/object/isr.o \
	  build/kernel/object/string.o \
	  build/kernel/object/stdlib.o \
	  build/kernel/object/heap.o \
	  build/kernel/object/fat.o \
	  build/kernel/object/process.o \
	  build/kernel/object/context_switch.o \
	  build/kernel/object/panic.o \
	  build/kernel/object/pmm.o \
	  build/kernel/object/dma.o \
	  build/kernel/object/e820.o \
	  build/kernel/object/tsc.o \
      -o build/kernel.elf

	@cat build/stub.bin build/kernel.elf > build/kernel.bin

	@ls -l build/kernel.elf

	@sudo losetup -fP build/fat.img 

	@sudo mcopy -i /dev/loop0p1 build/kernel.bin ::

	@sudo losetup -d /dev/loop0p1

fat:
	@sudo dd if=/dev/zero of=build/fat.img bs=512 count=4608
	@sudo dd if=stage1.bin of=build/fat.img bs=512 seek=0 conv=notrunc
	@sudo dd if=stage2.bin of=build/fat.img bs=512 seek=3 conv=notrunc
	@sudo losetup -fP build/fat.img
	@sudo cfdisk /dev/loop0
	@sudo mkfs.fat -F 12 /dev/loop0p1
	@sudo mount /dev/loop0p1 /mnt 
	@sudo cp build/kernel.bin /mnt/KERNEL.BIN
	@sudo umount /mnt
	@sudo losetup -d /dev/loop0

iso:
	@cp build/fat.img KittyyOS.iso

test:
	qemu-system-x86_64 -machine pc -m 512M -hda KittyyOS.iso -d int,cpu -no-reboot

nolog:
	qemu-system-x86_64 -machine pc -m 512M -hda KittyyOS.iso

xhci:
	qemu-system-x86_64 -device qemu-xhci -hda KittyyOS.iso -d int,cpu

ahci:
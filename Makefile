NEWLIB_INC = /home/Kittyy/newlib/x86_64-elf/include
CFLAGS = -ffreestanding -m64 -mno-red-zone -fno-stack-protector -O1 -Isrc/include -fno-asynchronous-unwind-tables
NASMFLAGS = -f bin
UEFI_CFLAGS = -ffreestanding -fno-stack-protector -fno-pie -fshort-wchar -mno-red-zone -Isrc/include
UEFI_CC = x86_64-w64-mingw32-gcc
UEFI_LDFLAGS = -nostdlib -Wl,--subsystem,10 -Wl,--entry,efi_entry


prepare:
	@mkdir -p build/iso
	@mkdir -p build/kernel/object
	@mkdir -p build/bootloader/object

bootloader:
	@echo "Compiling bootloader..."
	@nasm -f bin src/bootloader/stage2.asm -o build/stage2.bin

	@echo "Writing Bootloader into Image..."

	@sudo dd if=build/mbr.bin of=build/fat.img bs=512 seek=0 conv=notrunc
	@sudo dd if=build/stage2.bin of=build/fat.img bs=512 seek=3 conv=notrunc

	@echo "Compiling UEFI Bootloader"

	@$(UEFI_CC) $(UEFI_CFLAGS) -c src/efi/efi.c -o build/bootloader/object/efi.o
	@$(UEFI_CC) $(UEFI_CFLAGS) -c src/efi/gopefi.c -o build/bootloader/object/gopefi.o
	@$(UEFI_CC) $(UEFI_CFLAGS) -c src/efi/string.c -o build/bootloader/object/string.o
	@$(UEFI_CC) $(UEFI_CFLAGS) -c src/efi/uefi_fs.c -o build/bootloader/object/uefi_fs.o
	@nasm -f win64 src/efi/efi.asm -o build/bootloader/object/entry.o

	@$(UEFI_CC) $(UEFI_LDFLAGS) \
	  build/bootloader/object/entry.o \
	  build/bootloader/object/efi.o \
	  build/bootloader/object/gopefi.o \
	  build/bootloader/object/string.o \
	  build/bootloader/uefi_fs.o \
	  -o build/BOOTX64.EFI


kernel:
	@echo "Compiling header files..."

	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/header/string.c -o build/kernel/object/string.o # New

	@echo "Compiling assembly files..."

	@nasm $(NASMFLAGS) src/kernel/start/loader.asm -o build/stub.bin -l build/stub.lst
	@nasm -f elf64 src/kernel/start/start.asm -o build/kernel/object/start.o
	@nasm -f elf64 src/kernel/arch/x86_64/idt/isr.asm -o build/kernel/object/isr_stub.o
	@nasm -f elf64 src/kernel/arch/x86_64/gdt/gdt.asm -o build/kernel/object/gdt_load.o
	@nasm -f elf64 src/kernel/arch/x86_64/idt/irq0.asm -o build/kernel/object/irq0_stub.o
	@nasm -f elf64 src/kernel/arch/x86_64/proc/context_switch.asm -o build/kernel/object/context_switch.o

	@echo "Compiling C files..."

	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/fat.c -o build/kernel/object/fat.o 
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/heap/heap.c -o build/kernel/object/heap.o                                                                                                                                                                                                                                                                                                          
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/vga/vga.c -o build/kernel/object/vga.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/keyboard/keyboard.c -o build/kernel/object/keyboard.o 
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/ports.c -o build/kernel/object/ports.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/idt/isr.c -o build/kernel/object/isr.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/idt/idt.c -o build/kernel/object/idt.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/kernel.c -o build/kernel/object/kernel.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/panic/panic.c -o build/kernel/object/panic.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/gdt/gdt.c -o build/kernel/object/gdt.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/mm/mmu.c -o build/kernel/object/mmu.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/mm/alloc.c -o build/kernel/object/alloc.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/bios/bios.c -o build/kernel/object/bios.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/ata/ata.c -o build/kernel/object/ata_dma.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/cd.c -o build/kernel/object/cd.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/disk.c -o build/kernel/object/disk.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/storage_init.c -o build/kernel/object/storage_init.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/pci/pci.c -o build/kernel/object/pci.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/pmm/pmm.c -o build/kernel/object/pmm.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/dma/dma.c -o build/kernel/object/dma.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/bios/e820/e820.c -o build/kernel/object/e820.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/tsc/tsc.c -o build/kernel/object/tsc.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/info/cpu.c -o build/kernel/object/cpu.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/path.c -o build/kernel/object/path.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/vfs.c -o build/kernel/object/vfs.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/ramfs/ramfs.c -o build/kernel/object/ramfs.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/ramfs/vfs_ramfs.c -o build/kernel/object/vfs_ramfs.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/vfs_fat32.c -o build/kernel/object/vfs_fat32.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/vga/vbe.c -o build/kernel/object/vbe.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/tools.c -o build/kernel/object/tools.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/pit/pit.c -o build/kernel/object/pit.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/idt/irq0.c -o build/kernel/object/irq0.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/vga/kprint.c -o build/kernel/object/kprint.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/vga/kcolors.c -o build/kernel/object/kcolors.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/kernel_api.c -o build/kernel/object/kernel_api.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/syscall/syscalls.c -o build/kernel/object/syscalls.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/elf/elf.c -o build/kernel/object/elf.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/proc/scheduler.c -o build/kernel/object/scheduler.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/arch/x86_64/proc/context.c -o build/kernel/object/context.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/ahci/ahci.c -o build/kernel/object/ahci.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/ahci/fs.c -o build/kernel/object/fs.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/vga/gop.c -o build/kernel/object/gop.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/filesystem/devfs/devfs.c -o build/kernel/object/devfs.o
	@x86_64-elf-gcc $(CFLAGS) -c src/kernel/driver/tty/tty.c -o build/kernel/object/tty.o

	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/sys/scheduler.c -o build/kernel/object/scheduler_bin.o
	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/sys/load.c -o build/kernel/object/load.o
	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/sys/proc/proc.c -o build/kernel/object/proc_app.o
	@nasm -f elf64 src/apps/sys/scheduler.asm -o build/kernel/object/scheduler_asm.o
	@nasm -f elf64 src/apps/sys/entry.asm -o build/kernel/object/entry.o
	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/sys/shell.c -o build/kernel/object/shell.o

	@x86_64-elf-ld \
	  -static \
	  -T src/apps/sys/linker.ld \
	  -o build/kernel/sched.bin \
	  build/kernel/object/scheduler_bin.o \
	  build/kernel/object/context_switch.o \
	  build/kernel/object/proc_app.o \
	  build/kernel/object/load.o \
	  build/kernel/object/entry.o \
	  build/kernel/object/shell.o \
	  build/kernel/object/scheduler_asm.o 

	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/hh/hh.c -o build/kernel/object/hh.o

	@x86_64-elf-ld \
	  -static \
	  -T src/apps/hh/linker.ld \
	  -o build/kernel/hh.bin \
	  build/kernel/object/hh.o 

	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/userspace/usr.c -o build/kernel/object/usr.o

	@x86_64-elf-ld \
	  -static \
	  -T src/apps/userspace/linker.ld \
	  -o build/kernel/usr.bin \
	  build/kernel/object/usr.o

	@x86_64-elf-gcc $(CFLAGS) -fPIC -c src/apps/atom/atom.c -o build/kernel/object/atom.o

	@x86_64-elf-ld \
	  -static \
	  -T src/apps/atom/linker.ld \
	  -o build/kernel/atom.bin \
	  build/kernel/object/atom.o

	@echo "Compiling Kernel..."

	@x86_64-elf-ld -m elf_x86_64 \
	  -T linker.ld \
      build/kernel/object/start.o \
	  build/kernel/object/kernel.o \
	  build/kernel/object/isr_stub.o \
      build/kernel/object/idt.o \
      build/kernel/object/vga.o \
	  build/kernel/object/mmu.o \
	  build/kernel/object/disk.o \
	  build/kernel/object/storage_init.o \
	  build/kernel/object/alloc.o \
      build/kernel/object/keyboard.o \
	  build/kernel/object/cd.o \
	  build/kernel/object/ata_dma.o \
	  build/kernel/object/bios.o \
	  build/kernel/object/gdt.o \
	  build/kernel/object/gdt_load.o \
      build/kernel/object/ports.o \
      build/kernel/object/isr.o \
	  build/kernel/object/string.o \
	  build/kernel/object/heap.o \
	  build/kernel/object/fat.o \
	  build/kernel/object/panic.o \
	  build/kernel/object/pmm.o \
	  build/kernel/object/dma.o \
	  build/kernel/object/e820.o \
	  build/kernel/object/tsc.o \
	  build/kernel/object/cpu.o \
	  build/kernel/object/path.o \
	  build/kernel/object/vfs.o \
	  build/kernel/object/ramfs.o \
	  build/kernel/object/vfs_ramfs.o \
	  build/kernel/object/vfs_fat32.o \
	  build/kernel/object/vbe.o \
	  build/kernel/object/tools.o \
	  build/kernel/object/pit.o \
	  build/kernel/object/irq0.o \
	  build/kernel/object/irq0_stub.o \
	  build/kernel/object/kprint.o \
	  build/kernel/object/kcolors.o \
	  build/kernel/object/kernel_api.o \
	  build/kernel/object/syscalls.o \
	  build/kernel/object/elf.o \
	  build/kernel/object/scheduler.o \
	  build/kernel/object/context_switch.o \
	  build/kernel/object/context.o \
	  build/kernel/object/ahci.o \
	  build/kernel/object/pci.o \
	  build/kernel/object/fs.o \
	  build/kernel/object/gop.o \
	  build/kernel/object/devfs.o \
	  build/kernel/object/tty.o \
      -o build/kernel.elf

	@cat build/stub.bin build/kernel.elf > build/kernel.bin

	@ls -l build/kernel.elf

	@sudo losetup -fP build/fat.img 

	@sudo mcopy -i /dev/loop0p1 build/kernel.bin ::
	@sudo mount /dev/loop0p1 /mnt
	@sudo cp build/BOOTX64.EFI /mnt/EFI/BOOT/
	@sudo cp build/kernel.elf /mnt/
	@sudo umount /mnt

	@sudo mount /dev/loop0p2 /mnt 

	@sudo cp build/kernel/sched.bin /mnt/bin
	@sudo cp build/kernel/hh.bin /mnt/bin
	@sudo cp build/kernel/usr.bin /mnt/bin
	@sudo cp build/kernel/atom.bin /mnt/bin

	@sudo umount /mnt

	@sudo losetup -d /dev/loop0p1

fat:
	@sudo dd if=/dev/zero of=build/fat.img bs=1M count=64
	@sudo losetup -fP build/fat.img

	@sudo cfdisk /dev/loop0

	@sudo mkfs.fat -F 12 /dev/loop0p1

	@sudo mkfs.fat -F 32 /dev/loop0p2

	@sudo losetup -d /dev/loop0

	@sudo dd if=build/stage1.bin of=build/fat.img bs=512 count=1 conv=notrunc

	@sudo dd if=build/stage2.bin of=build/fat.img bs=512 seek=3 conv=notrunc

	@sudo losetup -fP build/fat.img

	@sudo dd if=build/kernel.bin of=build/fat.img seek=2085 conv=notrunc

	@sudo losetup -d /dev/loop0

iso:
	@cp build/fat.img KittyyOS.iso

test:
	qemu-system-x86_64 -machine pc -m 512M -hda KittyyOS.iso -d int,cpu -no-reboot

nolog:
	qemu-system-x86_64 -enable-kvm -cpu qemu64 -monitor stdio -machine pc -m 500M -hda KittyyOS.iso -no-reboot

xhci:
	qemu-system-x86_64 -device qemu-xhci -hda KittyyOS.iso

ahci:
	qemu-system-x86_64 -machine q35 -m 500M -drive file=build/fat.img,format=raw,if=none,id=disk0 -device ich9-ahci,id=ahci -device ide-hd,drive=disk0,bus=ahci.2

uefi:
	qemu-system-x86_64 -machine q35 -m 500M -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd build/fat.img
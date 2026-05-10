CC := x86_64-elf-gcc
LD := x86_64-elf-ld

DISK = /dev/sda

CFLAGS :=
LDFLAGS :=

KERNEL = src/kernel
DRIVERS = src/kernel/drivers
MM = src/kernel/mm
KERNEL_B = build/kernel
DRIVERS_B = build/kernel/drivers
MM_B = build/kernel/mm


override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -m64 \
    -march=x86-64 \
    -mabi=sysv \
    -mno-80387 \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mno-red-zone \
    -mcmodel=kernel \

override LDFLAGS += \
    -m elf_x86_64 \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    --gc-sections

all: bin/kernel.elf

initialize: clean
	-mkdir build
	-mkdir bin
	-mkdir lib
	-mkdir build/kernel
	-mkdir build/kernel/drivers
	-mkdir build/kernel/drivers/framebuffer
	-mkdir build/kernel/mm
	-mkdir build/kernel/mm/bootmm
	-mkdir disk
	-mkdir disk/efi
	-mkdir disk/root


clean:
	rm -rf build
	rm -rf bin
	-rm -rf disk


download_libs:
	@echo "Downloading libraries..."
	@echo "[0/1] Cloning Limine bootloader..."
	@cd lib && git clone https://github.com/Limine-Bootloader/Limine.git
	@echo "[1/1] Done!"


partition_disk:
	-@sudo umount ${DISK}1 2>/dev/null
	-@sudo umount ${DISK}2 2>/dev/null
	@echo 'Wiping partitions from ${DISK}...'
	@sudo wipefs -a ${DISK} > /dev/null
	@echo 'Creating partitions...'
	@printf 'label: gpt\nsize=1G,name=EFI,type=uefi\nsize=1G,name=ROOT\n' | sudo sfdisk ${DISK} > /dev/null
	@echo 'Creating filesystems...'
	@sudo mkfs.fat -F32 ${DISK}1 > /dev/null
	@sudo mkfs.fat -F32 ${DISK}2 > /dev/null


mount_disk:
	@echo 'Mounting partitions...'
	-@sudo mount /dev/sda1 disk/efi
	-@sudo mount /dev/sda2 disk/root


install_limine: mount_disk
	@echo "Installing Limine..."
	@sudo cp lib/limine/limine.conf disk/efi
	@sudo cp lib/limine/BOOTX64.EFI disk/efi


install_kernel:
	@echo "Installing kernel..."
	-@sudo mkdir disk/root/kernel
	@sudo cp bin/kernel.elf disk/root/kernel/kernel.elf


deploy: install_limine install_kernel


${KERNEL_B}/kernel.o: ${KERNEL}/main.c
	${CC} ${CFLAGS} -c -o $@ $<

${DRIVERS_B}/framebuffer/main.o: ${DRIVERS}/framebuffer/main.c
	${CC} ${CFLAGS} -c -o $@ $<

${DRIVERS_B}/framebuffer/colors.o: ${DRIVERS}/framebuffer/colors.c
	${CC} ${CFLAGS} -c -o $@ $<

${MM_B}/bootmm/main.o: ${MM}/bootmm/main.c
	${CC} ${CFLAGS} -c -o $@ $<



bin/kernel.elf: ${KERNEL_B}/kernel.o ${DRIVERS_B}/framebuffer/main.o ${DRIVERS_B}/framebuffer/colors.o ${MM_B}/bootmm/main.o ${KERNEL}/linker.lds
	${LD} ${LDFLAGS} -T ${KERNEL}/linker.lds -o $@ $(filter %.o,$^)

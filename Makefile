ROOT_DIR=$(CURDIR)
GNUK=$(ROOT_DIR)/gnuk-1.1.2/
OPENOCD_BIN?=~/Projects/openocd/build/bin

gnuk:
	cd $(GNUK)/src && \
    ./configure --vidpid=234b:0000 --target=OLIMEX_STM32_H103 --enable-keygen && \
	make -j$(J)

gnuk-flash-stlink:
	cd $(GNUK) && \
	cp src/build/gnuk.elf . && \
	sudo $(OPENOCD_BIN)/openocd -f interface/stlink-v2.cfg -f board/olimex_stm32_h103.cfg -f tool/openocd-script/write.tcl

gnuk-flash-versaloon:
	cp $(GNUK)/src/build/gnuk.elf . && \
	sudo $(OPENOCD_BIN)/openocd gdb_memory_map disable -f interface/vsllink-swd.cfg -f board/olimex_stm32_h103.cfg -f $(GNUK)/tool/openocd-script/write.tcl
	#sudo /home/cyc0/Projects/openocd/build/bin/openocd gdb_memory_map disable -f interface/vsllink-swd.cfg -f board/olimex_stm32_h103.cfg -f gnuk-1.1.2/tool/openocd-script/write.tcl

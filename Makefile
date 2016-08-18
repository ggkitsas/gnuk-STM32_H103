ROOT_DIR=$(CURDIR)
BUILD_DIR=$(ROOT_DIR)/build
GNUK=$(ROOT_DIR)/gnuk/
OPENOCD_BIN?=~/Projects/openocd/build/bin
J?=1

.PHONY: gnuk

gnuk: #$(GNUK)/src/build/gnuk.elf $(GNUK)/src/build/gnuk.bin
	cd $(GNUK)/src && \
    ./configure --vidpid=234b:0000 --target=OLIMEX_STM32_H103 && \
	make -j$(J)
	cp $(GNUK)/src/build/gnuk.elf $(GNUK)/src/build/gnuk.bin $(BUILD_DIR)

clean:
	cd $(GNUK)/src && make clean && make distclean

gnuk-flash-stlink:
	cd $(GNUK) && \
	cp src/build/gnuk.elf . && \
	sudo $(OPENOCD_BIN)/openocd -f interface/stlink-v2.cfg -f board/olimex_stm32_h103.cfg -f tool/openocd-script/write.tcl

gnuk-flash-versaloon:
	cp $(GNUK)/src/build/gnuk.elf . && \
	sudo $(OPENOCD_BIN)/openocd gdb_memory_map disable -f interface/vsllink-swd.cfg -f board/olimex_stm32_h103.cfg -f $(GNUK)/tool/openocd-script/write.tcl


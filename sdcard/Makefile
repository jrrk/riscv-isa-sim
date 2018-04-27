CXXFLAGS=-g -fPIC -DPIC -m64 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -Wredundant-decls -Wall -Wundef -Wwrite-strings -fno-strict-aliasing -fno-common -fwrapv  -Wendif-labels -Wmissing-include-dirs -Wempty-body -Wformat-security -Wformat-y2k -Winit-self -Wignored-qualifiers -Wtype-limits -fstack-protector-all 

obj_dir/Vusimv_top__ALL.a: obj_dir/Vusimv_top.mk
	make -C obj_dir -f Vusimv_top.mk CXXFLAGS="$(CXXFLAGS)"
	make -C obj_dir -f Vusimv_top.mk CXXFLAGS="$(CXXFLAGS)" verilated.o
	ar r $@ obj_dir/verilated.o
	ranlib $@

obj_dir/Vusimv_top.mk: usimv_top.sv sd_verilator_model.sv sd_crc_16.v sd_crc_7.v sd_cmd_serial_host.v
	 verilator -cc usimv_top.sv sd_verilator_model.sv sd_crc_16.v sd_crc_7.v sd_cmd_serial_host.v

clean:
	rm -rf Vusim_top obj_dir

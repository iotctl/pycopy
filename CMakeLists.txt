set(PYCOPY_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(PYCOPY_GENHDR_DIR ${PYCOPY_GENERATED_DIR}/genhdr)
set(PYCOPY_QSTR_H ${PYCOPY_GENHDR_DIR}/qstrdefs.generated.h)


# add_library(pycopy INTERFACE)

# target_include_directories(pycopy INTERFACE 
# 	ports/zephyr
# 	$<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>	
# )

# target_compile_definitions(pycopy INTERFACE
# 	$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>	
# 

# # file(GENERATE OUTPUT filename CONTENT "$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>")

# target_compile_options(pycopy INTERFACE
# 	$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>	
# )
# zephyr_interface_library_named(pycopy)

zephyr_library_named(pycopy)

zephyr_include_directories(
	.
	ports/zephyr
	${PYCOPY_GENERATED_DIR}
)

#target_sources(pycopy INTERFACE
zephyr_library_sources(
    py/argcheck.c
	py/asmarm.c
	py/asmbase.c
	py/asmthumb.c
	py/asmx64.c
	py/asmx86.c
	py/asmxtensa.c
	py/bc.c
	py/binary.c
	py/builtinevex.c
	py/builtinhelp.c
	py/builtinimport.c
	py/compile.c
	py/emitbc.c
	py/emitcommon.c
	py/emitglue.c
	py/emitinlinethumb.c
	py/emitinlinextensa.c
	py/emitnarm.c
	py/emitnative.c
	py/emitnthumb.c
	py/emitnx64.c
	py/emitnx86.c
	py/emitnxtensa.c
	py/formatfloat.c
	py/frozenmod.c
	py/gc.c
	py/lexer.c
	py/malloc.c
	py/map.c
	py/modarray.c
	py/modbuiltins.c
	py/modcmath.c
	py/modcollections.c
	py/modgc.c
	py/modio.c
	py/modmath.c
	py/modmicropython.c
	py/modstruct.c
	py/modsys.c
	py/modthread.c
	py/moduerrno.c
	py/mpprint.c
	py/mpstate.c
	py/mpz.c
	py/nativeglue.c
	py/nlr.c
	py/nlrsetjmp.c
	py/nlrthumb.c
	py/nlrx64.c
	py/nlrx86.c
	py/nlrxtensa.c
	py/objarray.c
	py/objattrtuple.c
	py/objbool.c
	py/objboundmeth.c
	py/obj.c
	py/objcell.c
	py/objclosure.c
	py/objcomplex.c
	py/objdeque.c
	py/objdict.c
	py/objenumerate.c
	py/objexcept.c
	py/objfilter.c
	py/objfloat.c
	py/objfun.c
	py/objgenerator.c
	py/objgetitemiter.c
	py/objint.c
	py/objint_longlong.c
	py/objint_mpz.c
	py/objlist.c
	py/objmap.c
	py/objmodule.c
	py/objnamedtuple.c
	py/objnone.c
	py/objobject.c
	py/objpolyiter.c
	py/objproperty.c
	py/objrange.c
	py/objreversed.c
	py/objset.c
	py/objsingleton.c
	py/objslice.c
	py/objstr.c
	py/objstringio.c
	py/objstrunicode.c
	py/objtuple.c
	py/objattrtuple.c
	py/objtype.c
	py/objzip.c
	py/opmethods.c
	py/parse.c
	py/parsenumbase.c
	py/parsenum.c
	py/persistentcode.c
	py/pystack.c
	py/qstr.c
	py/reader.c
	py/repl.c
	py/runtime.c
	py/runtime_utils.c
	py/scheduler.c
	py/scope.c
	py/sequence.c
	py/showbc.c
	py/smallint.c
	py/stackctrl.c
	py/stream.c
	py/unicode.c
	py/vm.c
	py/vstr.c
	py/warning.c
	lib/utils/pyexec.c
	lib/utils/stdout_helpers.c
	lib/utils/interrupt_char.c
	lib/mp-readline/readline.c
	extmod/modubinascii.c
	extmod/moduhashlib.c
	extmod/utime_mphal.c
	extmod/machine_signal.c
	extmod/machine_i2c.c
	extmod/virtpin.c
	extmod/vfs.c
	extmod/vfs_reader.c
	# zephyr
#ports/zephyr/main.c
	ports/zephyr/uart_core.c
	ports/zephyr/help.c
	ports/zephyr/modmachine.c
	ports/zephyr/machine_pin.c
	ports/zephyr/machine_i2c.c
	ports/zephyr/modzephyr.c
	ports/zephyr/modusocket.c
	ports/zephyr/moduos.c
	ports/zephyr/modutime.c
#ports/zephyr/vfs_zephyr.c
)

# get_property(flags TARGET pycopy PROPERTY INTERFACE_LINK_LIBRARIES)
# message("XXX: ${flags}")

# get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_LINK_LIBRARIES)
# message("XXX: ${flags}")

# get_property(xxx TARGET pycopy PROPERTY COMPILE_DEFINITIONS)
# message("AAA: ${xxx}")

# get_property(flags TARGET pycopy PROPERTY COMPILE_OPTIONS)
# message("BBB: ${xxx}")

get_property(ZEPHYR_INTERFACE_OPTIONS TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)
process_flags(C ZEPHYR_INTERFACE_OPTIONS PYCOPY_OPTIONS)

# get_property(PYCOPY_OPTIONS TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

# get_property(flags2 TARGET zephyr_interface PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
# message("XXX2: ${flags2}")

# message("XXX3: ")
# zephyr_library_named(pycopy)
# zephyr_include_directories($<TARGET_PROPERTY:pycopy,INTERFACE_INCLUDE_DIRECTORIES>)
# zephyr_include_directories(${PYCOPY_GENERATED_DIR})
# # target_include_directories(pycopy PRIVATE ports/zephyr)
# zephyr_library_sources($<TARGET_PROPERTY:pycopy,INTERFACE_SOURCES>)

# zephyr_library_sources(

# Basic idea is to have 2 targets
#
# pycopy-qstr - used to generate QSTRs, other modules can update its sources and includes as well as flags and definitions
# and still have ability to compile themselves as independent libraries without saturation of pycopy target
#
# pycopy is dependent on pycopy-qstr
#

# TODO pkral:
# pycopy-qstr must include all includes and sources as pycopy, this is copy-paste workaround
# so maybe we can define "base" interface for both "pycopy" and "pycopy-qstr" ?


add_custom_command(
	OUTPUT ${PYCOPY_QSTR_H}
	COMMAND ${CMAKE_CURRENT_LIST_DIR}/tools/cmake-qstr.py
	--compiler ${CMAKE_C_COMPILER}
	--flags "\"${PYCOPY_OPTIONS}\""
	--defs \"$<TARGET_PROPERTY:pycopy,COMPILE_DEFINITIONS>\"
	--includes \"$<TARGET_PROPERTY:pycopy,INCLUDE_DIRECTORIES>\"
	--sources \"$<TARGET_PROPERTY:pycopy,SOURCES>\"
	--outdir ${PYCOPY_GENHDR_DIR}
	COMMENT "Generating QSTRs"
	DEPENDS $<TARGET_PROPERTY:pycopy,INTERFACE_SOURCES>
	VERBATIM
)

add_custom_target(pycopy-qstr-gen DEPENDS ${PYCOPY_QSTR_H})
add_dependencies(pycopy-qstr-gen ${SYSCALL_LIST_H_TARGET} ${KOBJ_TYPES_H_TARGET})
add_dependencies(pycopy pycopy-qstr-gen)


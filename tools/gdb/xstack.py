import gdb
import xstack_arp

gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        xstack_arp.build_pretty_printer())

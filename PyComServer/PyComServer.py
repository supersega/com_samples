from ctypes import *
from comtypes import CLSCTX_LOCAL_SERVER, CLSCTX_INPROC_SERVER
from comtypes.server.localserver import REGCLS_MULTIPLEUSE
import sys

# Get wrapped code for RoyalPython.tlb,
# It was generated in RegPyComServer.bat file from RoyalPython.idl
from comtypes.client import GetModule
GetModule("RoyalPython.tlb")

# comtypes.gen.PyComServer was generated by GetModule above
# RoyalPython corresponds to the RoyalPython from RoyalPython.idl
from comtypes.gen.PyComServer import RoyalPython

# Implement COM server using auto generated type RoyalPython
class PyRoyalPython(RoyalPython):
    _reg_threading_ = "Both"
    _reg_progid_ = "PyComServer.RoyalPython.1"
    _reg_novers_progid_ = "PyComServer.RoyalPython"
    _reg_clsctx_ = CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER
    _reg_desc_ = "Royal python eats mammals"
    _regcls_ = REGCLS_MULTIPLEUSE

    def Eat(self):
        print("eat")

if __name__ == '__main__':
    # If some arguments passed to the script we call UseCommandLine what register/unregister COM server.
    if len(sys.argv) > 1:
        from comtypes.server.register import UseCommandLine
        UseCommandLine(PyRoyalPython)
    else:
        # Run the server.
        from win32com.server import localserver
        localserver.serve(['{F405A270-9088-4800-A5CD-A5E1DED02AB4}'])
import unittest
from ctypes import CFUNCTYPE
from ctypes import c_void_p
from ctypes import POINTER
from ctypes import c_int
from ctypes import cdll

""" This snippet shows the interoperability between 
python and the c library being created.  The c program
must implement the folling interface:
void init(pointer-to-callback)
void process(integer)
The init function will send the pointer to the 
python function as a callback.  The working code
for doing this on the C-side is in tpms.c and should
not need modification.  
The process function will take a 32 bit integer as 
the parameter to simluate the incoming ADC values.
Beware that it is up to this script to manage value
ranges since python doesn't handle some intrisic types
well.  
Note that to add a new test, the prefix 'test' must be
added to each test method name to be recognized by the
test runner.
"""

class Harness(unittest.TestCase):

    def setUp(self):
        """ setUp
        This method gets called before each test method.
        
        Attributes
        ----------
        clib:
            Reference to the loaded library 'object'.
        XMIT:
            Pointer definition from ctypes
        transmitted:
            Used to collect values in a callback from 
            the library.
        """
        self.clib = cdll.LoadLibrary('./libtpms.so')
        self.XMIT = CFUNCTYPE(c_void_p, POINTER(c_int))
        # self.clib.receive.argtypes = [POINTER(c_int), c_int]
        self.rx = []
        self.cmp_xmit = self.XMIT(self.transmit)
        # This first call into the C library can cause an exception.
        self.clib.init(self.cmp_xmit)


    def transmit(self, v):
        """Collect the value from the tpms library callback.
        v: wrapped value from C, take the [0] index to get the value
        """
        self.rx.append(v[0])
        # Simply uncomment print a console debug message with the value.
        # print('TRANSMIT CALLED {}'.format(v[0]))

    def test_simple_1(self):
        """ simple_test_1
        This test just calls into the library to process one
        value through it and expects one value back out.
        The demo tpms.c only echoes the value given back
        through the callback mechanism.
        """
        self.clib.process(100)
        self.assertEqual(100, self.rx[0])
		
    def test_simple_2(self):
        """ simple_test_2
        This test just calls into the library to process one
        value through it and expects one value back out.
        The demo tpms.c only echoes the value given back
        through the callback mechanism.
        """
        self.clib.process(33)
        self.assertEqual(33, self.rx[0])

    def test_array_1(self):
        """ test_array_1
        Test calling into a library with a byte array
        and receiving a byte array back.
        """
        # build python array
        arr =  [1,2,3,4,5]

        # allocates memory for an equivalent array in C 
        # and populates it with values from `arr`
        arr_c = (c_int * 5)(*arr)

        # Call the C function
        self.clib.receive(arr_c, c_int(5))      
        print('Returned buffer= {}'.format(self.rx))

    def test_fail(self):
        """ test_fail
        Shows how to use a couple of asserts and to demonstrate
        that multiple asserts in a method still count as 1 test.
        """
        self.assertFalse(False)
        self.assertTrue(True)

    def dump_list(self):
        """ dump_list
        Dumps the contents of the rx list to the console.
        """
        print("List contents={}".format(self.rx))


# Use this block to run as pytest
# Usage: python3 harness.py
if __name__ == '__main__':
    unittest.main()

'''
#  Use this block to run manually.
# Usage: python3 harness.py
harness = Harness()
harness.setUp()
harness.test_simple_1()
harness.setUp()
harness.test_simple_2()
# harness.setUp()
harness.dump_list()
harness.test_fail()
'''
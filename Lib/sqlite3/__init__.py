# pysqlite2/__init__.py: the pysqlite2 package.
#
# Copyright (C) 2005 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

from sqlite3.dbapi2 import *

def setup_rtree_support():
    """Import R*Tree support, if possible."""
    try:  # Check if this sqlite3 module was compiled with R*Tree support.
        import _sqlite3_rtree as rtree
    except ImportError:
        def wrapper(self, name, func):
            raise NotImplementedError("R*Tree support is not available")
    else:
        import functools
        @functools.wraps(rtree.create_query_function)
        def wrapper(self, name, func):
            rtree.create_query_function(self, name, func)

    Connection.create_rtree_query_function = wrapper
    Connection.create_rtree_query_function.__name__ = "create_rtree_query_function"
    Connection.create_rtree_query_function.__doc__ = wrapper.__doc__

setup_rtree_support()
del setup_rtree_support

# bpo-42264: OptimizedUnicode was deprecated in Python 3.10.  It's scheduled
# for removal in Python 3.12.
def __getattr__(name):
    if name == "OptimizedUnicode":
        import warnings
        msg = ("""
            OptimizedUnicode is deprecated and will be removed in Python 3.12.
            Since Python 3.3 it has simply been an alias for 'str'.
        """)
        warnings.warn(msg, DeprecationWarning, stacklevel=2)
        return str
    raise AttributeError(f"module 'sqlite3' has no attribute '{name}'")

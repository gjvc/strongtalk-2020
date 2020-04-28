#! /usr/bin/env python3

import ply
import lark

grammar = '''


'''



parser = lark.Lark( grammar )
parser.parse( input_text )



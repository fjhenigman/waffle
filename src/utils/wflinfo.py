#! /usr/bin/env python

import json
import os
import sys
import subprocess

def xlate(a):
    if a.startswith('WAFFLE_PLATFORM_'):
        return a[16:].lower()
    if a.startswith('WAFFLE_CONTEXT_'):
        return dict(
            OPENGL     = 'gl',
            OPENGL_ES1 = 'gles1',
            OPENGL_ES2 = 'gles2',
            OPENGL_ES3 = 'gles3',
        ).get(a[15:], a)
    return a

'''
wflinfo = os.path.join(os.path.dirname(sys.argv[0]), 'wflinfo')
cmd = [ wflinfo ] + sys.argv[1:]
proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
out, err = proc.communicate()
if proc.returncode:
    sys.exit(proc.returncode)
'''
out = sys.stdin.read()

j = json.loads(out)

print 'Waffle platform:', j['generic']['waffle']['platform'][16:].lower()
print 'Waffle api:', j['generic']['waffle']['api'][19:].lower()
print 'OpenGL vendor string:', j['generic']['opengl']['vendor']
print 'OpenGL renderer string:', j['generic']['opengl']['renderer']
print 'OpenGL version string:', j['generic']['opengl']['version']

#verbose
print 'OpenGL shading language version string:', j['generic']['opengl']['shading_language_version']
print 'OpenGL extensions:', ' '.join(j['generic']['opengl']['extensions'])

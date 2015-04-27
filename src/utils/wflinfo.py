#! /usr/bin/env python

import json
import os
import sys
import subprocess

def platform(s):
    if s.startswith('WAFFLE_PLATFORM_'):
        s = s[16:].lower()
    return s

def api(s):
    if s.startswith('WAFFLE_CONTEXT_'):
        s = dict(
            OPENGL     = 'gl',
            OPENGL_ES1 = 'gles1',
            OPENGL_ES2 = 'gles2',
            OPENGL_ES3 = 'gles3',
        ).get(s[15:], s)
    return s

'''
wflinfo = os.path.join(os.path.dirname(sys.argv[0]), 'wflinfo')
cmd = [ wflinfo ] + sys.argv[1:]
proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
out, err = proc.communicate()
if proc.returncode:
    sys.exit(proc.returncode)
'''

def get(*key):
    v = j
    for i in key:
        try:
            v = v[i]
        except:
            return ''
    return v

out = sys.stdin.read()
j = json.loads(out)
j = get('generic')

print 'Waffle platform:', platform(get('waffle', 'platform'))
print 'Waffle api:', api(get('waffle', 'api'))
print 'OpenGL vendor string:', get('opengl', 'vendor')
print 'OpenGL renderer string:', get('opengl', 'renderer')
print 'OpenGL version string:', get('opengl', 'version')

#verbose
print 'OpenGL shading language version string:', get('shading_language_version')
print 'OpenGL extensions:', ' '.join(get('extensions'))

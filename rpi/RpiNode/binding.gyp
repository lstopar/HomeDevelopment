{
	'variables': {
		'QMINER_PATH%': '../../../lib/qminer',
		'SHARED_PATH%': '../../shared/RF24Protocol'
	},
    'target_defaults': {
        'default_configuration': 'Release',
        'configurations': {
            'Debug': {
                'defines': [
                    'DEBUG'
                ]
            },
            'Release': {
                'defines': [
                    'NDEBUG'
                ]
            }
        },
        'defines': [
        ],
        'libraries': [
        	'-lrt',
        	'-luuid',
        	'-fopenmp',
        	'-lwiringPi',
        	'-lrf24-bcm',
        	'-lrf24network'
        ],
        # GCC flags
        'cflags_cc!': [ '-fno-rtti', '-fno-exceptions' ],
        'cflags_cc': [ '-std=c++0x', '-frtti', '-fexceptions' ],
        'cflags': [ '-Wno-deprecated-declarations', '-fopenmp' ]
    },
    'targets': [
        {
            'target_name': 'rpinode',
            'sources': [
            	'src/rpi.h',
            	'src/rpi.cpp',
            	'<(SHARED_PATH)/protocol.h',
            	'<(SHARED_PATH)/protocol.cpp',
            	'src/rpinode.h',
            	'src/rpinode.cpp',
            	'src/threads.h',
            	'src/threads.cpp',
                '<(QMINER_PATH)/src/nodejs/nodeutil.h',
                '<(QMINER_PATH)/src/nodejs/nodeutil.cpp'
            ],
            'include_dirs': [
                'src/',
                '<(SHARED_PATH)/',
                '<(QMINER_PATH)/src/nodejs/',
                '<(QMINER_PATH)/src/glib/base/',
                '<(QMINER_PATH)/src/glib/mine/',
                '<(QMINER_PATH)/src/glib/misc/'
            ],
            'defines': [
            ],
            'dependencies': [
                'glib'
            ]
        },
        {
            # node qminer module
            'target_name': 'test',
            'type': 'executable',
            'sources': [
                'test.cpp'
            ],
            'include_dirs': [
            ],
            'defines': [],
            'dependencies': []
        },
        {
            # glib library
            'target_name': 'glib',
            'type': 'static_library',
            'sources': [
                '<(QMINER_PATH)/src/glib/base/base.h',
                '<(QMINER_PATH)/src/glib/base/base.cpp',
                '<(QMINER_PATH)/src/glib/mine/mine.h',
                '<(QMINER_PATH)/src/glib/mine/mine.cpp'
            ],
            'include_dirs': [
                '<(QMINER_PATH)/src/glib/base/',
                '<(QMINER_PATH)/src/glib/mine/',
                '<(QMINER_PATH)/src/glib/misc/',
                '<(QMINER_PATH)/src/third_party/sole/'
            ],
            'defines': [
            ]
        }
    ]
}

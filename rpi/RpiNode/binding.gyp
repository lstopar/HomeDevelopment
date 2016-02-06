{
	'variables': {
		'QMINER_PATH%': '../../../lib/qminer',
		'RF24_PATH%': 'lib/RF24/librf24-rpi/librf24-bcm'
	},
    'target_defaults': {
        'default_configuration': 'Release',
        'configurations': {
            'Debug': {
                'defines': [
                    'DEBUG',
                ],
            },
            'Release': {
                'defines': [
                    'NDEBUG'
                ],
            }
        },
        'defines': [
        ],
        # hack for setting xcode settings based on example from
        # http://src.chromium.org/svn/trunk/o3d/build/common.gypi
        'target_conditions': [        
            ['OS=="mac"', {
                'xcode_settings': {
                    'MACOSX_DEPLOYMENT_TARGET': '10.7',
                    'GCC_ENABLE_CPP_RTTI': 'YES',
                    'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                    'OTHER_CFLAGS': [ '-std=c++11', '-stdlib=libc++' ]
                },
            }],
        ],
        'conditions': [
            # operating system specific parameters
            ['OS == "linux"', {
                'libraries': [ '-lrt', '-luuid', '-fopenmp', '-lwiringPi' ],
                # GCC flags
                'cflags_cc!': [ '-fno-rtti', '-fno-exceptions' ],
                'cflags_cc': [ '-std=c++0x', '-frtti', '-fexceptions' ],
                'cflags': [ '-Wno-deprecated-declarations', '-fopenmp' ]
            }],
            ['OS == "win"', {
                'msbuild_toolset': 'v120',
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        #'RuntimeTypeInfo': 'true',      # /GR  : this should work but doesn't get picked up
                        #'ExceptionHandling': 1,         # /EHsc: this should work but doesn't get picked up
                        'OpenMP': 'true',
                        "AdditionalOptions": [ "/EHsc /GR" ] # release mode displays D9025 warnings, which is a known issue https://github.com/nodejs/node-gyp/issues/335
                    },
                    'VCLinkerTool': {
                        'SubSystem' : 1, # Console
                        'AdditionalOptions': [  ]
                    },
                },
            }],
            ['OS == "mac"', {
                "default_configuration": "Release",
                "configurations": {
                    "Debug": {
                        "defines": [
                            "DEBUG",
                        ],
                        "xcode_settings": {
                            "GCC_OPTIMIZATION_LEVEL": "0",
                            "GCC_GENERATE_DEBUGGING_SYMBOLS": "YES"
                        }
                    },
                    "Release": {
                        "defines": [
                            "NDEBUG"
                        ],
                        "xcode_settings": {
                            "GCC_OPTIMIZATION_LEVEL": "3",
                            "GCC_GENERATE_DEBUGGING_SYMBOLS": "NO",
                            "DEAD_CODE_STRIPPING": "YES",
                            "GCC_INLINES_ARE_PRIVATE_EXTERN": "YES"
                        }
                    }
                }
            }]
        ]
    },
    'targets': [
        {
            'target_name': 'rpinode',
            'sources': [
            	'src/rpi.h',
            	'src/rpi.cpp',
            	'src/rpinode.h',
            	'src/rpinode.cpp',
            	'src/threads.h',
            	'src/threads.cpp',
                '<(QMINER_PATH)/src/nodejs/nodeutil.h',
                '<(QMINER_PATH)/src/nodejs/nodeutil.cpp'
            ],
            'include_dirs': [
                'src/',
                '<(QMINER_PATH)/src/nodejs/',
                '<(QMINER_PATH)/src/glib/base/',
                '<(QMINER_PATH)/src/glib/mine/',
                '<(QMINER_PATH)/src/glib/misc/',
                '<(RF24_PATH)/'
            ],
            'defines': [
            ],
            'dependencies': [
                'glib',
                'rf24'
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
            'defines': [
            ],
            'dependencies': [
            ]
        },
        {
            # RF24 radio library
            'target_name': 'rf24',
            'type': 'static_library',
            'sources': [
                '<(RF24_PATH)/RF24.h',
                '<(RF24_PATH)/RF24.cpp'
            ],
            'include_dirs': [
                '<(RF24_PATH)/'
            ],
            'defines': [
            ],
            'conditions': [
	            # operating system specific parameters
	            ["target_arch=='arm'", {
	              	'cflags_cc': [
	                	'-Ofast',
	                	'-mfpu=vfp',
						'-mfloat-abi=hard',
						'-march=armv6zk',
						'-mtune=arm1176jzf-s'
					]
	            }]
            ]
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

{
	'variables': {
		'QMINER_PATH%': '../../../lib/qminer',
		'SHARED_PATH%': '../../shared/RF24Protocol',
		'ENOCEAN_PATH%': '../../../lib/EnOcean/EOLink'
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
            	'src/enocean.h',
            	'src/enocean.cpp',
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
                '<(ENOCEAN_PATH)/',
            	'<(ENOCEAN_PATH)/Includes/',
            	'<(ENOCEAN_PATH)/Profiles/',
                '<(QMINER_PATH)/src/nodejs/',
                '<(QMINER_PATH)/src/glib/base/',
                '<(QMINER_PATH)/src/glib/mine/',
                '<(QMINER_PATH)/src/glib/misc/',
                '<(QMINER_PATH)/src/glib/concurrent/'
            ],
            'defines': [
            ],
            'dependencies': [
                'glib',
                'eolink'
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
                '<(QMINER_PATH)/src/glib/mine/mine.cpp',
                '<(QMINER_PATH)/src/glib/concurrent/thread.h',
                '<(QMINER_PATH)/src/glib/concurrent/thread.cpp',
            ],
            'include_dirs': [
                '<(QMINER_PATH)/src/glib/base/',
                '<(QMINER_PATH)/src/glib/mine/',
                '<(QMINER_PATH)/src/glib/misc/',
                '<(QMINER_PATH)/src/glib/concurrent/',
                '<(QMINER_PATH)/src/third_party/sole/'
            ],
            'defines': [
            ]
        },
        {
            # EOLink library
            'target_name': 'eolink',
            'type': 'static_library',
            'sources': [
                '<(ENOCEAN_PATH)/eoLink.h',
                
                '<(ENOCEAN_PATH)/Includes/eoGateway.h',
                '<(ENOCEAN_PATH)/Gateway/eoGateway.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoSerialCommand.h',
                '<(ENOCEAN_PATH)/SerialCommands/eoSerialCommand.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoDebug.h',
                '<(ENOCEAN_PATH)/Misc/eoDebug.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoISerialize.h',
                '<(ENOCEAN_PATH)/Storage/eoISerialize.cpp',
                '<(ENOCEAN_PATH)/Includes/eoStorageManager.h',
                '<(ENOCEAN_PATH)/Storage/eoStorageManager.cpp',
                '<(ENOCEAN_PATH)/Includes/eoArchive.h',
                '<(ENOCEAN_PATH)/Storage/eoArchive.cpp',
                '<(ENOCEAN_PATH)/Includes/eoArchiveTXT.h',
                '<(ENOCEAN_PATH)/Storage/eoArchiveTXT.cpp',
                '<(ENOCEAN_PATH)/Includes/eoHeader.h',
                '<(ENOCEAN_PATH)/Storage/eoHeader.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoPacket.h',
                '<(ENOCEAN_PATH)/Packet/eoPacket.cpp',
                '<(ENOCEAN_PATH)/Includes/eoLinuxPacketStream.h',
                '<(ENOCEAN_PATH)/Packet/eoLinuxPacketStream.cpp',
                '<(ENOCEAN_PATH)/Includes/eoPacketStream.h',
                '<(ENOCEAN_PATH)/Packet/eoPacketStream.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoTelegram.h',
                '<(ENOCEAN_PATH)/Message/eoTelegram.cpp',
                '<(ENOCEAN_PATH)/Includes/eoMessage.h',
                '<(ENOCEAN_PATH)/Message/eoMessage.cpp',
                '<(ENOCEAN_PATH)/Includes/eoAbstractMessage.h',
                '<(ENOCEAN_PATH)/Message/eoAbstractMessage.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoDeviceManager.h',
                '<(ENOCEAN_PATH)/Device/eoDeviceManager.cpp',
                '<(ENOCEAN_PATH)/Includes/eoDevice.h',
                '<(ENOCEAN_PATH)/Device/eoDevice.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoTeachInModule.h',
                '<(ENOCEAN_PATH)/TeachIn/eoTeachInModule.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoConverter.h',
                '<(ENOCEAN_PATH)/Misc/eoConverter.cpp',
                '<(ENOCEAN_PATH)/Includes/eoProc.h',
                '<(ENOCEAN_PATH)/Misc/eoProc.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoTimer.h',
                '<(ENOCEAN_PATH)/Timer/eoTimer.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoIFilter.h',
                '<(ENOCEAN_PATH)/Filter/eoIFilter.cpp',
                '<(ENOCEAN_PATH)/Includes/eoFilterFactory.h',
                '<(ENOCEAN_PATH)/Filter/eoFilterFactory.cpp',
                '<(ENOCEAN_PATH)/Includes/eoIDFilter.h',
                '<(ENOCEAN_PATH)/Filter/eoIDFilter.cpp',
                '<(ENOCEAN_PATH)/Includes/eodBmFilter.h',
                '<(ENOCEAN_PATH)/Filter/eodBmFilter.cpp',
                
                '<(ENOCEAN_PATH)/Includes/eoProfileFactory.h',
                '<(ENOCEAN_PATH)/Profiles/eoProfileFactory.cpp',
                '<(ENOCEAN_PATH)/Includes/eoProfile.h',
                '<(ENOCEAN_PATH)/Profiles/eoProfile.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEProfile.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEProfile.cpp',
                '<(ENOCEAN_PATH)/Includes/eoA5EEProfile.h',
                '<(ENOCEAN_PATH)/Profiles/eoA5EEProfile.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_F603xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_F603xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoF6EEProfile.h',
                '<(ENOCEAN_PATH)/Profiles/eoF6EEProfile.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_F602xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_F602xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_F604xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_F604xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_F610xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_F610xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_A502xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_A502xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_A504xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_A504xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_A506xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_A506xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_A507xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_A507xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_A510xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_A510xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_A512xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_A512xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_D500xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_D500xx.cpp',
                '<(ENOCEAN_PATH)/Includes/eoEEP_D201xx.h',
                '<(ENOCEAN_PATH)/Profiles/eoEEP_D201xx.cpp'
            ],
            'include_dirs': [
                '<(ENOCEAN_PATH)/',
                '<(ENOCEAN_PATH)/Includes/'
            ],
            'defines': [
            	'POSIX'
            ]
        }
    ]
}

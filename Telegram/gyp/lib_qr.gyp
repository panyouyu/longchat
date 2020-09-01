# This file is part of Telegram Desktop,
# the official desktop application for the Telegram messaging service.
#
# For license and copyright information please follow this link:
# https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL

{
  'includes': [
    'common.gypi',
  ],
  'targets': [{
    'target_name': 'lib_qr',
    'type': 'static_library',
    'dependencies': [
    ],
    'includes': [
      'common.gypi',
      'qt.gypi',
    ],
    'defines': [
    ],
    'variables': {
      'qr_src_loc': '../ThirdParty/QR/cpp',
      'official_build_target%': '',
    },
    'include_dirs': [
      '../ThirdParty/QR/cpp',
      '../SourceFiles',
    ],
    'sources': [
      '<(qr_src_loc)/BitBuffer.cpp',
      '<(qr_src_loc)/BitBuffer.hpp',
      '<(qr_src_loc)/QrCode.cpp',
      '<(qr_src_loc)/QrCode.hpp',
      '<(qr_src_loc)/QrSegment.cpp',
      '<(qr_src_loc)/QrSegment.hpp',
    ],
  }],
}

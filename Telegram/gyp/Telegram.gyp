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
    'target_name': 'LongChat',
    'variables': {
      'variables': {
        'libs_loc': '../../../Libraries',
      },
      'libs_loc': '<(libs_loc)',
      'src_loc': '../SourceFiles',
      'res_loc': '../Resources',
      'submodules_loc': '../ThirdParty',
      'minizip_loc': '<(submodules_loc)/minizip',
      'sp_media_key_tap_loc': '<(submodules_loc)/SPMediaKeyTap',
      'emoji_suggestions_loc': '<(submodules_loc)/emoji_suggestions',
      'style_files': [
        '<(res_loc)/colors.palette',
        '<(res_loc)/basic.style',
        '<(src_loc)/boxes/boxes.style',
        '<(src_loc)/calls/calls.style',
        '<(src_loc)/dialogs/dialogs.style',
        '<(src_loc)/export/view/export.style',
        '<(src_loc)/history/history.style',
        '<(src_loc)/info/info.style',
        '<(src_loc)/intro/intro.style',
        '<(src_loc)/media/view/mediaview.style',
        '<(src_loc)/media/player/media_player.style',
        '<(src_loc)/overview/overview.style',
        '<(src_loc)/passport/passport.style',
        '<(src_loc)/profile/profile.style',
        '<(src_loc)/settings/settings.style',
        '<(src_loc)/chat_helpers/chat_helpers.style',
        '<(src_loc)/ui/widgets/widgets.style',
        '<(src_loc)/window/window.style',
      ],
      'langpacks': [
        'en',
        'de',
        'es',
        'it',
        'nl',
        'ko',
        'pt-BR',
      ],
      'build_defines%': '',
      'list_sources_command': 'python <(DEPTH)/list_sources.py --input <(DEPTH)/telegram_sources.txt --replace src_loc=<(src_loc)',
      'pch_source': '<(src_loc)/stdafx.cpp',
      'pch_header': '<(src_loc)/stdafx.h',
    },
    'includes': [
      'common_executable.gypi',
      'telegram_qrc.gypi',
      'telegram_win.gypi',
      'telegram_mac.gypi',
      'telegram_linux.gypi',
      'openssl.gypi',
      'qt.gypi',
      'qt_moc.gypi',
      'qt_rcc.gypi',
      'codegen_rules.gypi',
      'pch.gypi',
    ],

    'dependencies': [
      'codegen.gyp:codegen_emoji',
      'codegen.gyp:codegen_lang',
      'codegen.gyp:codegen_numbers',
      'codegen.gyp:codegen_style',
      'tests/tests.gyp:tests',
      'utils.gyp:Updater',
	  'utils.gyp:Packer',
      '../ThirdParty/libtgvoip/libtgvoip.gyp:libtgvoip',
      'crl.gyp:crl',
      'lib_base.gyp:lib_base',
      'lib_export.gyp:lib_export',
	  'lib_qr.gyp:lib_qr',
      'lib_storage.gyp:lib_storage',
    ],

    'defines': [
      'AL_LIBTYPE_STATIC',
      'AL_ALEXT_PROTOTYPES',
      'TGVOIP_USE_CXX11_LIB',
      'XXH_INLINE_ALL',
      'TDESKTOP_API_ID=<(api_id)',
      'TDESKTOP_API_HASH=<(api_hash)',
      '<!@(python -c "for s in \'<(build_defines)\'.split(\',\'): print(s)")',
    ],

    'include_dirs': [
      '<(src_loc)',
      '<(SHARED_INTERMEDIATE_DIR)',
      '<(libs_loc)/breakpad/src',
      '<(libs_loc)/lzma/C',
      '<(libs_loc)/zlib',
      '<(libs_loc)/ffmpeg',
      '<(libs_loc)/openal-soft/include',
      '<(libs_loc)/opus/include',
      '<(libs_loc)/range-v3/include',
      '<(minizip_loc)',
      '<(sp_media_key_tap_loc)',
      '<(emoji_suggestions_loc)',
      '<(submodules_loc)/GSL/include',
      '<(submodules_loc)/variant/include',
      '<(submodules_loc)/crl/src',
	  '<(submodules_loc)/QR/cpp',
      '<(submodules_loc)/xxHash'
    ],
    'sources': [
      '<@(qrc_files)',
      '<@(style_files)',
      '<!@(<(list_sources_command) <(qt_moc_list_sources_arg))',
      'telegram_sources.txt',
	  '<(res_loc)/langs/cloud_lang.strings',
      '<(res_loc)/export_html/css/style.css',
      '<(res_loc)/export_html/js/script.js',
      '<(res_loc)/export_html/images/back.png',
      '<(res_loc)/export_html/images/back@2x.png',
    ],
    'sources!': [
      '<!@(<(list_sources_command) <(qt_moc_list_sources_arg) --exclude_for <(build_os))',
    ],
    'conditions': [
      [ '"<(official_build_target)" != ""', {
        'defines': [
          'TDESKTOP_OFFICIAL_TARGET=<(official_build_target)',
        ],
        'dependencies': [
          'utils.gyp:Packer',
        ],
      }], [ 'build_mac', {
        'mac_bundle': '1',
        'mac_bundle_resources': [
          '<!@(python -c "for s in \'<@(langpacks)\'.split(\' \'): print(\'<(res_loc)/langs/\' + s + \'.lproj/Localizable.strings\')")',
          '../Telegram/Images.xcassets',
        ],
      }], [ 'build_macstore', {
        'mac_sandbox': 1,
        'mac_sandbox_development_team': '6N38VWS5BX',
        'product_name': 'Telegram Desktop',
        'sources': [
          '../Telegram/Telegram Desktop.entitlements',
        ],
      }],
    ],
  }],
}

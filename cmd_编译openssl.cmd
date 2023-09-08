参考https://github.com/openssl/openssl/blob/master/NOTES-WINDOWS.md

https://www.openssl.org/source/
下载openssl-1.1.1v.tar.gz (SHA256) (PGP sign) (SHA1)
释放到目录 C:\openssl-1.1.1v\openssl-1.1.1v

https://www.activestate.com/products/perl/
https://www.nasm.us/


mkdir c:\openssl_lib\static_lib\win32
mkdir c:\openssl_lib\shared_dll\win32
mkdir c:\openssl_lib\shared_dll\win64
mkdir c:\openssl_lib\static_lib\win64

VS2015 x86 Native Tools Command Prompt
VS2015 x64 Native Tools Command Prompt

进入目录C:\openssl-1.1.1v\openssl-1.1.1v

32位
perl Configure VC-WIN32 no-asm no-shared --prefix=C:\openssl_lib\static_lib\win32
perl Configure VC-WIN32 no-asm --prefix=C:\openssl_lib\shared_dll\win32

64位
perl Configure VC-WIN64A no-asm no-shared --prefix=C:\openssl_lib\static_lib\win64
perl Configure VC-WIN64A no-asm --prefix=C:\openssl_lib\shared_dll\win64


Debug版
debug-VC-WIN32
debug-VC-WIN64A


nmake clean

perl Configure VC-WIN64A no-asm no-shared --prefix=C:\openssl_lib\static_lib\win64
nmake
nmake install



perl Configure VC-WIN64A no-asm --prefix=C:\openssl_lib\shared_dll\win64
nmake
nmake install
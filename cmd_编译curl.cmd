https://www.cnblogs.com/damizhou/p/11744096.html


选择编译
DLL Release - DLL Windows SSPI - DLL WinIDN
这样windows下访问Https就可以用windows自带的根证书

curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);//winssl编译时使用windows自带的根证书
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
SSH 方式拉取github 代码
1.打开 Windows PowerShell
2.ssh-keygen  生成本地key
3.cd .ssh 
ls

点github 我的头像，Settings==>>SSH and GPG keys==>New SSH key
复制id_rsa.pub里面的内容到Key 栏目里面，Title 随便取，然后点击Add SSH key
添加后，就可以正常的提交代码了

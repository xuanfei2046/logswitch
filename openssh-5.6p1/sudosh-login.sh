#!/bin/bash - 
#===============================================================================
#        AUTHOR:  XuanFei
#         EMAIL:  xuanfei2046@163.com
#
#       CREATED:  2010年11月25日 02时16分07秒 CST
#
#          FILE:  sudosh-login.sh
#         USAGE:  ./sudosh-login.sh 
#       OPTIONS:  ---
#   DESCRIPTION:  sudosh login
#         NOTES:  ---
#===============================================================================

#start sudosh
#sudosh;

#remote log server
read -p "Please input the IP:" name;
ssh  loger@$name;


exit;


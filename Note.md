# install docker

```
# ubuntu 22.04

sudo apt -y install docker-ce

sudo snap install docker

# 修改源(这里采用的snap安装)
/var/snap/docker/current/config/daemon.json
snap stop docker
snap start docker

# handle "Got permission denied while trying to connect to the Docker daemon socket at unix:///var/run/docker.sock: Get "http://%2Fvar%2Frun%2Fdocker.sock/v1.24/version": dial unix /var/run/docker.sock: connect: permission denied"

sudo groupadd docker     #添加docker用户组
sudo gpasswd -a $USER docker     #将登陆用户加入到docker用户组中
newgrp docker     #更新用户组
sudo docker ps    #测试docker命令是否可以使用sudo正常使用


# 安装mysql开发环境
sudo apt -y install libmysqlclient-dev

# 创建mysql容器或直接安装
docker pull mysql:latest

# start contain
docker run \
--name=IM_SQL \
--env="MYSQL_ROOT_PASSWORD=123456" \
--publish 13306:13306 \
-d \
mysql
```

# 建表语句

```
create table IF NOT EXISTS `user`(
        id int PRIMARY KEY NOT NULL AUTO_INCREMENT,
        name varchar(64),
        password varchar(64),
        state varchar(64)
        )ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;;
create index user_unique_name on user(name);

create table IF NOT EXISTS `friends`(id int PRIMARY KEY NOT NULL AUTO_INCREMENT,
        userid int NOT NULL,
        friendid int NOT NULL
        )ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;;
create index friends_from_to on `friends`(userid, friendid);

# 未考虑大文件问题
create table IF NOT EXISTS `offlineMsg`(
        id int PRIMARY KEY NOT NULL AUTO_INCREMENT,
        fromid int NOT NULL,
        toid int NOT NULL,
        message varchar(1024)
        )ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;;

create index offlineMsg_by_userid on offlineMsg(userid);

create table IF NOT EXISTS `offlineGroupMsg`(
        id int PRIMARY KEY NOT NULL AUTO_INCREMENT,
        fromid int NOT NULL,
        groupid int NOT NULL,
        toid int NOT NULL,
        message varchar(1024)
        )ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;;

```

## 群聊
create table IF NOT EXISTS `allGroups`(
        id int PRIMARY KEY NOT NULL AUTO_INCREMENT,
        groupname varchar(64),
        groupdesc varchar(64)
        )ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;;


create table IF NOT EXISTS `groupMembers`(
        groupid int NOT NULL,
        userid int NOT NULL,
        role int NOT NULL
        )ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;;

# 安装hiredis

```
git clone --depth=1 https://github.com/redis/hiredis.git
make
sudo make install
```

# nginx配置

```
https://docs.nginx.com/nginx/admin-guide/load-balancer/tcp-udp-load-balancer/
```

# 安装protobuf
```
PROTOC_VERSION=$(curl -s "https://api.github.com/repos/protocolbuffers/protobuf/releases/latest" | grep -Po '"tag_name": "v\K[0-9.]+')

curl -Lo protoc.zip "https://github.com/protocolbuffers/protobuf/releases/latest/download/protoc-${PROTOC_VERSION}-linux-x86_64.zip"

sudo unzip -q protoc.zip bin/protoc -d /usr/local

sudo chmod a+x /usr/local/bin/protoc

protoc --version

# 简单的方案
```

# 安装boost
```
sudo apt install libboost-all-dev
```

# 安装mysql

```
mkdir -p mysql && cd $_ && mkdir {conf,data}
```

创建my.cnf
```
#创建my.cnf
# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 
#
 
# This program is free software; you can redistribute it and/or modify
 
# it under the terms of the GNU General Public License as published by
 
# the Free Software Foundation; version 2 of the License.
 
#
 
# This program is distributed in the hope that it will be useful,
 
# but WITHOUT ANY WARRANTY; without even the implied warranty of
 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 
# GNU General Public License for more details.
 
#
 
# You should have received a copy of the GNU General Public License
 
# along with this program; if not, write to the Free Software
 
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 
#
 
# The MySQL Server configuration file.
 
#
 
# For explanations see
 
# http://dev.mysql.com/doc/mysql/en/server-system-variables.html
 
[mysqld]
 
pid-file = /var/run/mysqld/mysqld.pid
 
socket = /var/run/mysqld/mysqld.sock
 
datadir = /var/lib/mysql
 
secure-file-priv= NULL
 
# Disabling symbolic-links is recommended to prevent assorted security risks
 
symbolic-links=0
 
# Custom config should go here
 
!includedir /etc/mysql/conf.d/
 
max_connections=1000
wait_timeout=120
interactive_timeout=300
 
lower_case_table_names=1
```

## 创建mysql容器
```
docker run --restart=unless-stopped -d --name mysql -p 3306:3306 -v /home/$USER/mysql/conf/my.cnf:/etc/mysql/my.cnf -v /home/$USER/mysql/data:/var/lib/mysql -e MYSQL_ROOT_PASSWORD=root mysql
```

## 处理远程连接问题
```
docker exec -it mysql /bin/bash

use mysql

ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '新密码';

FLUSH PRIVILEGES;

# 这里我们创建新数据库和新用户,不使用root用户访问
select user,host from user;
create database chat;
create user 'dev'@'%' identified by '123456';
grant all privileges on chat.* to 'dev'@'%';
flush privileges;

# 远程登陆
mysql -u USERNAME -pPASSWORD -h HOSTNAMEORIP DATABASENAME
```


# 安装redis

```
docker pull redis
docker run --restart=unless-stopped -itd --name IM_redis -p 6379:6379 redis
# docker update --restart=always 容器id或容器名
```


# tcp package结构
1字节magic number: 0xAA => 1010 1010
4字节protobuf 长度:


# 负载均衡

服务器模式(存在瓶颈), 但是用nginx做的L4负载均衡比较简单...(不支持动态扩展)
todo: 服务发现

## reference

+ [微服务：负载均衡算法](http://ldaysjun.com/2019/01/20/Microservice/micro4/)
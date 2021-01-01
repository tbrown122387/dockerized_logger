# Interactive Brokers Gateway in Docker 

## Overview

I cannot share my data with students, so this repo is to help students download their own data from an Interactive Brokers account. It will also be used as a stepping stone for writing a dockerized execution client. 

This repo modifies the work [here](https://github.com/dvasdekis/ib-gateway-docker-gcp).

## Quick start guide

On a host machine that you have shell access to (remote or local), clone this repo, change the config files, then type 

```docker-compose up``` 

or 

```docker-compose up -d``` 

if you want to run everything in the background. Depending on how you installed Docker and Docker-Compose, you might need to add a `sudo` in front of those commands above. The first time you do this it will take a while--that is because it is building the entire Docker image. The good news is that the second time around everything is much quicker.

### Config Files

These are what you need to change before you `docker-compose up` everything. The config files are the ones with sensitive information, so be sure not to publicly share your edited version of this code. They are
 
1. your Interactive Brokers user id and password are going to be stored in `.env` 

2. your mysql database information is stored in `dockerized_logger/log_app/ib_client/IBJts/samples/Cpp/TestCppClient/mysql_config.txt`

3. the symbols you are interested in tracking (at the moment this is futures only!) are in `dockerized_logger/log_app/ib_client/IBJts/samples/Cpp/TestCppClient/tickers.txt`

Depending on how you installed Docker and Docker-Compose, you might need to add a `sudo` in front that command. The first time you do this it will take a while--that is because it is installing the operating system and all the software. The good news is that the second time around it's much quicker. 



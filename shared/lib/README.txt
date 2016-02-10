# clone the following two libraries

git clone https://github.com/TMRh20/RF24.git
git clone https://github.com/TMRh20/RF24Network.git

# open RF24Network_config.h and comment lines that define DISABLE_FRAGMENTATION and ENABLE_DYNAMIC_PAYLOADS
# to enable neighbour ACKs open RF24Network.cpp and
# - change line XXX from writeFast to write
# - commend line ok = radio.txStandBy(txTimeout); in write_to_pipe

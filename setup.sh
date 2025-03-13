apt update
apt install libcurl4-openssl-dev nlohmann-json3-dev -y
g++ -o cppbank cppbank.cpp -lcurl
cp cppbank /bin 
echo -e "\n\n\e[34m Type cppbank on terminal to start\e[0m"
 
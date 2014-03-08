echo *+*+*+* make test programs:
make all
echo *+*+*+* generate sample files sample0 to sample9:
chmod +x gen.sh
./gen.sh
ls
sleep 20
echo *+*+*+* run tests:
chmod +x try.sh
./try.sh


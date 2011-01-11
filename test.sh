#/bin/bash/

rm *.txt
rm *.bin
rm *.egz

clear

if [ "$2" ] && [ "$2" -gt 0 ]; then
    
    ./build/bin/egz -d -f -c "test-files/$1"
    
else
    
    ./build/bin/egz -c -f "test-files/$1"
    
fi

if [ "$3" ] && [ "$3" -gt 0 ]; then
    
    ./build/bin/egz -d -f -x "$1.egz"
    
else
    
    ./build/bin/egz -x -f "$1.egz"
    
fi


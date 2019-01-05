#!/usr/local/bin/bash
yup() {
    make
    make config
    make clean
    read -p "Please enter the number of blocks: " num_blocks
    if [[ $num_blocks =~ ([0-9]+) ]]; then
        :
    else
        echo "Not a valid number!"
        exit
    fi
    read -p "Please enter the block size: " block_size
    if [[ $block_size =~ ([0-9]+) ]]; then
        :
    else
        echo "Not a valid number!"
        exit
    fi
    rm disk.img
    ./config $num_blocks $block_size
    read -p "Enter the directory you want to mount to: " dir
    if [ -e $dir ]
    then
        ./fileSystem $dir
    else
        mkdir $dir
        ./fileSystem $dir
        exit
    fi

}

nope() {
    read -p "Enter the directory you want to mount to: " dir
    if [ -e $dir ]
    then
        ./fileSystem $dir
    else
        mkdir $dir
        ./fileSystem $dir
        exit
    fi
}


if [ -f "disk.img" ]; then
    read -p "disk.img already exists do you want to overwrite it? (y/n)" yn
    case $yn in
        [Yy]* ) yup;;
        [Nn]* ) nope;;
        * ) echo "Please enter y/n";;
    esac
else
    yup;
fi


#REMOTE_ACCOUNT=pi@10.0.0.11
#REMOTE_ACCOUNT=pi@10.0.0.199
REMOTE_ACCOUNT=pi@10.0.0.158

REMOTE_PATH=projects/head

function upload {
    echo "$REMOTE_ACCOUNT:$REMOTE_PATH"
    rsync -r . "$REMOTE_ACCOUNT:$REMOTE_PATH"
}

function web {
    upload
    ssh $REMOTE_ACCOUNT "cd $REMOTE_PATH ; sudo python ./web.py"
}

function bweb {
    upload
    ssh $REMOTE_ACCOUNT "cd $REMOTE_PATH/visionModule ; cmake . ; make ; cd .. ; sudo python ./web.py"
}

function bweb2 {
    upload
    ssh $REMOTE_ACCOUNT "cd $REMOTE_PATH ; ./build.sh cbuild ; sudo python ./web.py"
}


function pi_shutdown {
    ssh $REMOTE_ACCOUNT "sudo shutdown -h now"
}

function clean_local {
    rm -rf ../visionModuleLocal
    mkdir ../visionModuleLocal
    cd ../visionModuleLocal
    cp ../head/visionModule/CMakeLists.win CMakeLists.txt
    cmake .
    cd ../head
}

function build_local {
    cd ../visionModuleLocal
    make
    cd ../head
}

function run_local {
    python web.py local
}

function run_motor_test {
    upload
    ssh $REMOTE_ACCOUNT "cd $REMOTE_PATH ; sudo python A1116.py"
}

function test_vision {
    python tests/testCamera.py
}

function dumps {
	rsync $REMOTE_ACCOUNT:$REMOTE_PATH/../dumps/* ../dumps
	rsync $REMOTE_ACCOUNT:$REMOTE_PATH/../metadata/* ../metadata
}

case $1 in
upload)
    upload
    ;;
web)
    web
    ;;
bweb)
    bweb2
    ;;
shutdown)
    pi_shutdown
    ;;
local)
    run_local
    ;;
    
cbuild)
    clean_local
    build_local
    ;;
build)
    build_local
    ;;
blocal)
    build_local
    run_local
    ;;
bblocal)
    clean_local
    build_local
    run_local
    ;;
motor)
    run_motor_test
    ;;
vision)
    build_local
    test_vision
    ;;
dumps)
    dumps
    ;;
esac

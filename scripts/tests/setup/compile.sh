# The following variables are passed in CompilationTest:
# - COMPILATION_SUBDIR
# - CONFIG
# - CHECKOUT_TAG

buildsDir="/mnt/builds"
thisBuildDir="$buildsDir/$COMPILATION_SUBDIR"

mkdir -p $thisBuildDir
cd $thisBuildDir
git clone ~/repos/9ld/

pushd 9ld
git checkout $CHECKOUT_TAG
CHECKOUT_COMMIT=$(git rev-parse HEAD)
make CONFIG=$CONFIG
make python-pb
# leave only necessary files in build directory
cp --parents -r web/ mdm/ bin/$CONFIG sql/ scripts/ ssl/ c-icap/ ../
popd

# clean workspace
rm -rf 9ld

# install in home directory
rm -rf ~/bin ~/web ~/sql ~/scripts ~/ssl
cp -r web bin  sql  scripts  ssl ~

# save configuration
echo $COMPILATION_SUBDIR > ~/lastCompilationSubdir
echo $CONFIG > ~/lastCompilationConfig
echo $CHECKOUT_TAG > ~/lastCompilationTag
echo $CHECKOUT_COMMIT > ~/lastCompilationCommit

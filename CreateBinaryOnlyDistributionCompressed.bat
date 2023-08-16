cmd /c BuildMediaCompressed.bat
md .\BinaryOnlyBuild
robocopy .\ .\BinaryOnlyBuild\ /E /XF *.7z BuildCode.bat CreateBinaryOnlyDistribution*.bat .git* MetaData.bin TextureData.bin /XD 4096 8192 src buildx build .git libs BinaryOnlyBuild BinaryOnlyBuild.old BinaryOnlyBuild.old.old buildx.old
REM "C:\Program Files\7-Zip\7z.exe" a BinaryOnlyBuildCompressed.7z .\BinaryOnlyBuild\* 

setlocal
pushd bin
FOR /f "tokens=* delims=" %%A in ('timestamp') do @set "ds_ts=%%A"
DirectStorageSample_DX12.exe {"directstorage": false, "profile": true, "profileOutputPath":"DSOffCompress_%ds_ts%.csv", "initializeAgs": false %*}
popd
setlocal
pushd bin
FOR /f "tokens=* delims=" %%A in ('timestamp') do @set "ds_ts=%%A"
DirectStorageSample_DX12.exe {"directstorage": true, "profile": true, "profileOutputPath":"DSOnCPUDecompression_%ds_ts%.csv", "iotiming":true, "initializeAgs": false, "disablegpudecompression":true %*}
popd
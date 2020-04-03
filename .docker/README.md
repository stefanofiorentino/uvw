# Run tests with docker

In order to run test we start from an ubuntu 18.04 image, then we install required dependency and than we issue: 
```
docker build \
    --build-arg CACHE_DATE=$(date +%Y-%m-%d:%H:%M:%S) \
    -t uvw:ubsan_on_asan_on \
    .docker/ubsan_on_asan_on
```
Substitute the image name accordingly.
# Cleanup
```
docker rmi uvw:ubsan_on_asan_on
```
Substitute the image name accordingly.
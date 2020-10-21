#ifdef UVW_AS_LIB
#include "fs.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE void FileReq::fsOpenCallback(uv_fs_t *req) {
    auto ptr = reserve(req);

    if(req->result < 0) {
        ptr->publish(ErrorEvent{req->result});
    } else {
        ptr->file = static_cast<uv_file>(req->result);
        ptr->publish(FsEvent<Type::OPEN>{req->path});
    }
}


UVW_INLINE void FileReq::fsCloseCallback(uv_fs_t *req) {
    auto ptr = reserve(req);

    if(req->result < 0) {
        ptr->publish(ErrorEvent{req->result});
    } else {
        ptr->file = BAD_FD;
        ptr->publish(FsEvent<Type::CLOSE>{req->path});
    }
}


UVW_INLINE void FileReq::fsReadCallback(uv_fs_t *req) {
    auto ptr = reserve(req);

    if(req->result < 0) {
        ptr->publish(ErrorEvent{req->result});
    } else {
        ptr->publish(FsEvent<Type::READ>{req->path, std::move(ptr->current), static_cast<std::size_t>(req->result)});
    }
}


UVW_INLINE FileReq::~FileReq() noexcept {
    uv_fs_req_cleanup(get());
}


UVW_INLINE void FileReq::close() {
    cleanupAndInvoke(&uv_fs_close, parent(), get(), file, &fsCloseCallback);
}


UVW_INLINE bool FileReq::closeSync() {
    auto req = get();

    cleanupAndInvokeSync(&uv_fs_close, parent(), req, file);

    if(req->result >= 0) {
        file = BAD_FD;
    }

    return !(req->result < 0);
}


UVW_INLINE void FileReq::open(std::string path, Flags<FileOpen> flags, int mode) {
    cleanupAndInvoke(&uv_fs_open, parent(), get(), path.data(), flags, mode, &fsOpenCallback);
}


UVW_INLINE bool FileReq::openSync(std::string path, Flags<FileOpen> flags, int mode) {
    auto req = get();

    cleanupAndInvokeSync(&uv_fs_open, parent(), req, path.data(), flags, mode);

    if(req->result >= 0) {
        file = static_cast<uv_file>(req->result);
    }

    return !(req->result < 0);
}


UVW_INLINE void FileReq::read(int64_t offset, unsigned int len) {
    current = std::unique_ptr<char[]>{new char[len]};
    buffer = uv_buf_init(current.get(), len);
    uv_buf_t bufs[] = {buffer};
    cleanupAndInvoke(&uv_fs_read, parent(), get(), file, bufs, 1, offset, &fsReadCallback);
}


UVW_INLINE std::pair<bool, std::pair<std::unique_ptr<const char[]>, std::size_t>> FileReq::readSync(int64_t offset, unsigned int len) {
    current = std::unique_ptr<char[]>{new char[len]};
    buffer = uv_buf_init(current.get(), len);
    uv_buf_t bufs[] = {buffer};
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_read, parent(), req, file, bufs, 1, offset);
    bool err = req->result < 0;
    return std::make_pair(!err, std::make_pair(std::move(current), err ? 0 : std::size_t(req->result)));
}


UVW_INLINE void FileReq::write(std::unique_ptr<char[]> buf, unsigned int len, int64_t offset) {
    current = std::move(buf);
    uv_buf_t bufs[] = {uv_buf_init(current.get(), len)};
    cleanupAndInvoke(&uv_fs_write, parent(), get(), file, bufs, 1, offset, &fsResultCallback<Type::WRITE>);
}


UVW_INLINE void FileReq::write(char *buf, unsigned int len, int64_t offset) {
    uv_buf_t bufs[] = {uv_buf_init(buf, len)};
    cleanupAndInvoke(&uv_fs_write, parent(), get(), file, bufs, 1, offset, &fsResultCallback<Type::WRITE>);
}


UVW_INLINE std::pair<bool, std::size_t> FileReq::writeSync(std::unique_ptr<char[]> buf, unsigned int len, int64_t offset) {
    current = std::move(buf);
    uv_buf_t bufs[] = {uv_buf_init(current.get(), len)};
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_write, parent(), req, file, bufs, 1, offset);
    bool err = req->result < 0;
    return std::make_pair(!err, err ? 0 : std::size_t(req->result));
}


UVW_INLINE void FileReq::stat() {
    cleanupAndInvoke(&uv_fs_fstat, parent(), get(), file, &fsStatCallback<Type::FSTAT>);
}


UVW_INLINE std::pair<bool, Stat> FileReq::statSync() {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_fstat, parent(), req, file);
    return std::make_pair(!(req->result < 0), req->statbuf);
}


UVW_INLINE void FileReq::sync() {
    cleanupAndInvoke(&uv_fs_fsync, parent(), get(), file, &fsGenericCallback<Type::FSYNC>);
}


UVW_INLINE bool FileReq::syncSync() {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_fsync, parent(), req, file);
    return !(req->result < 0);
}


UVW_INLINE void FileReq::datasync() {
    cleanupAndInvoke(&uv_fs_fdatasync, parent(), get(), file, &fsGenericCallback<Type::FDATASYNC>);
}


UVW_INLINE bool FileReq::datasyncSync() {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_fdatasync, parent(), req, file);
    return !(req->result < 0);
}


UVW_INLINE void FileReq::truncate(int64_t offset) {
    cleanupAndInvoke(&uv_fs_ftruncate, parent(), get(), file, offset, &fsGenericCallback<Type::FTRUNCATE>);
}


UVW_INLINE bool FileReq::truncateSync(int64_t offset) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_ftruncate, parent(), req, file, offset);
    return !(req->result < 0);
}


UVW_INLINE void FileReq::sendfile(FileHandle out, int64_t offset, std::size_t length) {
    cleanupAndInvoke(&uv_fs_sendfile, parent(), get(), out, file, offset, length, &fsResultCallback<Type::SENDFILE>);
}


UVW_INLINE std::pair<bool, std::size_t> FileReq::sendfileSync(FileHandle out, int64_t offset, std::size_t length) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_sendfile, parent(), req, out, file, offset, length);
    bool err = req->result < 0;
    return std::make_pair(!err, err ? 0 : std::size_t(req->result));
}


UVW_INLINE void FileReq::chmod(int mode) {
    cleanupAndInvoke(&uv_fs_fchmod, parent(), get(), file, mode, &fsGenericCallback<Type::FCHMOD>);
}


UVW_INLINE bool FileReq::chmodSync(int mode) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_fchmod, parent(), req, file, mode);
    return !(req->result < 0);
}


UVW_INLINE void FileReq::futime(FsRequest::Time atime, FsRequest::Time mtime) {
    cleanupAndInvoke(&uv_fs_futime, parent(), get(), file, atime.count(), mtime.count(), &fsGenericCallback<Type::FUTIME>);
}


UVW_INLINE bool FileReq::futimeSync(FsRequest::Time atime, FsRequest::Time mtime) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_futime, parent(), req, file, atime.count(), mtime.count());
    return !(req->result < 0);
}


UVW_INLINE void FileReq::chown(Uid uid, Gid gid) {
    cleanupAndInvoke(&uv_fs_fchown, parent(), get(), file, uid, gid, &fsGenericCallback<Type::FCHOWN>);
}


UVW_INLINE bool FileReq::chownSync(Uid uid, Gid gid) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_fchown, parent(), req, file, uid, gid);
    return !(req->result < 0);
}


UVW_INLINE FileReq::operator FileHandle() const noexcept {
    return file;
}


UVW_INLINE void FsReq::fsReadlinkCallback(uv_fs_t *req) {
    auto ptr = reserve(req);

    if(req->result < 0) {
        ptr->publish(ErrorEvent{req->result});
    } else {
        ptr->publish(FsEvent<Type::READLINK>{req->path, static_cast<char *>(req->ptr), static_cast<std::size_t>(req->result)});
    }
}


UVW_INLINE void FsReq::fsReaddirCallback(uv_fs_t *req) {
    auto ptr = reserve(req);

    if(req->result < 0) {
        ptr->publish(ErrorEvent{req->result});
    } else {
        auto *dir = static_cast<uv_dir_t *>(req->ptr);
        ptr->publish(FsEvent<Type::READDIR>{dir->dirents[0].name, static_cast<EntryType>(dir->dirents[0].type), !req->result});
    }
}


UVW_INLINE FsReq::~FsReq() noexcept {
    uv_fs_req_cleanup(get());
}


UVW_INLINE void FsReq::unlink(std::string path) {
    cleanupAndInvoke(&uv_fs_unlink, parent(), get(), path.data(), &fsGenericCallback<Type::UNLINK>);
}


UVW_INLINE bool FsReq::unlinkSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_unlink, parent(), req, path.data());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::mkdir(std::string path, int mode) {
    cleanupAndInvoke(&uv_fs_mkdir, parent(), get(), path.data(), mode, &fsGenericCallback<Type::MKDIR>);
}


UVW_INLINE bool FsReq::mkdirSync(std::string path, int mode) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_mkdir, parent(), req, path.data(), mode);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::mkdtemp(std::string tpl) {
    cleanupAndInvoke(&uv_fs_mkdtemp, parent(), get(), tpl.data(), &fsGenericCallback<Type::MKDTEMP>);
}


UVW_INLINE std::pair<bool, const char *> FsReq::mkdtempSync(std::string tpl) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_mkdtemp, parent(), req, tpl.data());
    return std::make_pair(!(req->result < 0), req->path);
}


UVW_INLINE void FsReq::mkstemp(std::string tpl) {
    cleanupAndInvoke(&uv_fs_mkstemp, parent(), get(), tpl.data(), &fsResultCallback<Type::MKSTEMP>);
}


UVW_INLINE std::pair<bool, std::pair<std::string, std::size_t>> FsReq::mkstempSync(std::string tpl) {
    std::pair<bool, std::pair<std::string, std::size_t>> ret{false, {}};
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_mkdtemp, parent(), req, tpl.data());
    ret.first = !(req->result < 0);

    if(ret.first) {
        ret.second.first = req->path;
        ret.second.second = static_cast<std::size_t>(req->result);
    }

    return ret;
}


UVW_INLINE void FsReq::lutime(std::string path, Time atime, Time mtime) {
    cleanupAndInvoke(&uv_fs_lutime, parent(), get(), path.data(), atime.count(), mtime.count(), &fsGenericCallback<Type::LUTIME>);
}


UVW_INLINE bool FsReq::lutimeSync(std::string path, Time atime, Time mtime) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_lutime, parent(), req, path.data(), atime.count(), mtime.count());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::rmdir(std::string path) {
    cleanupAndInvoke(&uv_fs_rmdir, parent(), get(), path.data(), &fsGenericCallback<Type::RMDIR>);
}


UVW_INLINE bool FsReq::rmdirSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_rmdir, parent(), req, path.data());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::scandir(std::string path, int flags) {
    cleanupAndInvoke(&uv_fs_scandir, parent(), get(), path.data(), flags, &fsResultCallback<Type::SCANDIR>);
}


UVW_INLINE std::pair<bool, std::size_t> FsReq::scandirSync(std::string path, int flags) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_scandir, parent(), req, path.data(), flags);
    bool err = req->result < 0;
    return std::make_pair(!err, err ? 0 : std::size_t(req->result));
}


UVW_INLINE std::pair<bool, std::pair<FsReq::EntryType, const char *>> FsReq::scandirNext() {
    std::pair<bool, std::pair<EntryType, const char *>> ret{false, { EntryType::UNKNOWN, nullptr }};

    // we cannot use cleanupAndInvokeSync because of the return value of uv_fs_scandir_next
    uv_fs_req_cleanup(get());
    auto res = uv_fs_scandir_next(get(), dirents);

    if(UV_EOF != res) {
        ret.second.first = static_cast<EntryType>(dirents[0].type);
        ret.second.second = dirents[0].name;
        ret.first = true;
    }

    return ret;
}


UVW_INLINE void FsReq::stat(std::string path) {
    cleanupAndInvoke(&uv_fs_stat, parent(), get(), path.data(), &fsStatCallback<Type::STAT>);
}


UVW_INLINE std::pair<bool, Stat> FsReq::statSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_stat, parent(), req, path.data());
    return std::make_pair(!(req->result < 0), req->statbuf);
}


UVW_INLINE void FsReq::lstat(std::string path) {
    cleanupAndInvoke(&uv_fs_lstat, parent(), get(), path.data(), &fsStatCallback<Type::LSTAT>);
}


UVW_INLINE std::pair<bool, Stat> FsReq::lstatSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_lstat, parent(), req, path.data());
    return std::make_pair(!(req->result < 0), req->statbuf);
}


UVW_INLINE void FsReq::statfs(std::string path) {
    cleanupAndInvoke(&uv_fs_statfs, parent(), get(), path.data(), &fsStatfsCallback);
}


UVW_INLINE std::pair<bool, Statfs> FsReq::statfsSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_statfs, parent(), req, path.data());
    return std::make_pair(!(req->result < 0), *static_cast<uv_statfs_t *>(req->ptr));
}


UVW_INLINE void FsReq::rename(std::string old, std::string path) {
    cleanupAndInvoke(&uv_fs_rename, parent(), get(), old.data(), path.data(), &fsGenericCallback<Type::RENAME>);
}


UVW_INLINE bool FsReq::renameSync(std::string old, std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_rename, parent(), req, old.data(), path.data());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::copyfile(std::string old, std::string path, Flags<CopyFile> flags) {
    cleanupAndInvoke(&uv_fs_copyfile, parent(), get(), old.data(), path.data(), flags, &fsGenericCallback<Type::COPYFILE>);
}


UVW_INLINE bool FsReq::copyfileSync(std::string old, std::string path, Flags<CopyFile> flags) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_copyfile, parent(), get(), old.data(), path.data(), flags);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::access(std::string path, int mode) {
    cleanupAndInvoke(&uv_fs_access, parent(), get(), path.data(), mode, &fsGenericCallback<Type::ACCESS>);
}


UVW_INLINE bool FsReq::accessSync(std::string path, int mode) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_access, parent(), req, path.data(), mode);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::chmod(std::string path, int mode) {
    cleanupAndInvoke(&uv_fs_chmod, parent(), get(), path.data(), mode, &fsGenericCallback<Type::CHMOD>);
}


UVW_INLINE bool FsReq::chmodSync(std::string path, int mode) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_chmod, parent(), req, path.data(), mode);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::utime(std::string path, FsRequest::Time atime, FsRequest::Time mtime) {
    cleanupAndInvoke(&uv_fs_utime, parent(), get(), path.data(), atime.count(), mtime.count(), &fsGenericCallback<Type::UTIME>);
}


UVW_INLINE bool FsReq::utimeSync(std::string path, FsRequest::Time atime, FsRequest::Time mtime) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_utime, parent(), req, path.data(), atime.count(), mtime.count());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::link(std::string old, std::string path) {
    cleanupAndInvoke(&uv_fs_link, parent(), get(), old.data(), path.data(), &fsGenericCallback<Type::LINK>);
}


UVW_INLINE bool FsReq::linkSync(std::string old, std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_link, parent(), req, old.data(), path.data());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::symlink(std::string old, std::string path, Flags<SymLink> flags) {
    cleanupAndInvoke(&uv_fs_symlink, parent(), get(), old.data(), path.data(), flags, &fsGenericCallback<Type::SYMLINK>);
}


UVW_INLINE bool FsReq::symlinkSync(std::string old, std::string path, Flags<SymLink> flags) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_symlink, parent(), req, old.data(), path.data(), flags);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::readlink(std::string path) {
    cleanupAndInvoke(&uv_fs_readlink, parent(), get(), path.data(), &fsReadlinkCallback);
}


UVW_INLINE std::pair<bool, std::pair<const char *, std::size_t>> FsReq::readlinkSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_readlink, parent(), req, path.data());
    bool err = req->result < 0;
    return std::make_pair(!err, std::make_pair(static_cast<char *>(req->ptr), err ? 0 : std::size_t(req->result)));
}


UVW_INLINE void FsReq::realpath(std::string path) {
    cleanupAndInvoke(&uv_fs_realpath, parent(), get(), path.data(), &fsGenericCallback<Type::REALPATH>);
}


UVW_INLINE std::pair<bool, const char *> FsReq::realpathSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_realpath, parent(), req, path.data());
    return std::make_pair(!(req->result < 0), req->path);
}


UVW_INLINE void FsReq::chown(std::string path, Uid uid, Gid gid) {
    cleanupAndInvoke(&uv_fs_chown, parent(), get(), path.data(), uid, gid, &fsGenericCallback<Type::CHOWN>);
}


UVW_INLINE bool FsReq::chownSync(std::string path, Uid uid, Gid gid) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_chown, parent(), req, path.data(), uid, gid);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::lchown(std::string path, Uid uid, Gid gid) {
    cleanupAndInvoke(&uv_fs_lchown, parent(), get(), path.data(), uid, gid, &fsGenericCallback<Type::LCHOWN>);
}


UVW_INLINE bool FsReq::lchownSync(std::string path, Uid uid, Gid gid) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_lchown, parent(), req, path.data(), uid, gid);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::opendir(std::string path) {
    cleanupAndInvoke(&uv_fs_opendir, parent(), get(), path.data(), &fsGenericCallback<Type::OPENDIR>);
}


UVW_INLINE bool FsReq::opendirSync(std::string path) {
    auto req = get();
    cleanupAndInvokeSync(&uv_fs_opendir, parent(), req, path.data());
    return !(req->result < 0);
}


UVW_INLINE void FsReq::closedir() {
    auto req = get();
    auto *dir = static_cast<uv_dir_t *>(req->ptr);
    cleanupAndInvoke(&uv_fs_closedir, parent(), req, dir, &fsGenericCallback<Type::CLOSEDIR>);
}


UVW_INLINE bool FsReq::closedirSync() {
    auto req = get();
    auto *dir = static_cast<uv_dir_t *>(req->ptr);
    cleanupAndInvokeSync(&uv_fs_closedir, parent(), req, dir);
    return !(req->result < 0);
}


UVW_INLINE void FsReq::readdir() {
    auto req = get();
    auto *dir = static_cast<uv_dir_t *>(req->ptr);
    dir->dirents = dirents;
    dir->nentries = 1;
    cleanupAndInvoke(&uv_fs_readdir, parent(), req, dir, &fsReaddirCallback);
}


UVW_INLINE std::pair<bool, std::pair<FsReq::EntryType, const char *>> FsReq::readdirSync() {
    auto req = get();
    auto *dir = static_cast<uv_dir_t *>(req->ptr);
    dir->dirents = dirents;
    dir->nentries = 1;
    cleanupAndInvokeSync(&uv_fs_readdir, parent(), req, dir);
    return {req->result != 0, { static_cast<EntryType>(dirents[0].type), dirents[0].name }};
}


UVW_INLINE OSFileDescriptor FsHelper::handle(FileHandle file) noexcept {
    return uv_get_osfhandle(file);
}


UVW_INLINE FileHandle FsHelper::open(OSFileDescriptor descriptor) noexcept {
    return uv_open_osfhandle(descriptor);
}

template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::UNKNOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CUSTOM>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::OPEN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CLOSE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::READ>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::WRITE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::SENDFILE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::STAT>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LSTAT>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FSTAT>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FTRUNCATE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::UTIME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FUTIME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::ACCESS>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CHMOD>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FCHMOD>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FSYNC>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FDATASYNC>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::UNLINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::RMDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::MKDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::MKDTEMP>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::RENAME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::SCANDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::SYMLINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::READLINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CHOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FCHOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::REALPATH>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::COPYFILE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LCHOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::OPENDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::READDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CLOSEDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::STATFS>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::MKSTEMP>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LUTIME>>;

template class uvw::Flags<enum uvw::details::UVFileOpenFlags> uvw::Flags<enum uvw::details::UVFileOpenFlags>::operator|(enum uvw::details::UVFileOpenFlags)const noexcept;
template class UVW_EXTERN uvw::Flags<enum uvw::details::UVFileOpenFlags>;

#define EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(EVENT) \
template typename \
uvw::Emitter<\
    class uvw::FileReq,\
    struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>,\
struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>,\
    struct uvw::FsEvent<uvw::details::UVFsType::OPEN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::READ>,\
    struct uvw::FsEvent<uvw::details::UVFsType::WRITE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::STAT>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::UTIME>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>,\
    struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>,\
    struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>,\
    struct uvw::FsEvent<uvw::details::UVFsType::RENAME>,\
    struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::READLINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>,\
    struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::READDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::STATFS>,\
    struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>,\
    struct uvw::ErrorEvent>::Handler< EVENT >::Connection \
uvw::Emitter<\
    class uvw::FileReq,\
    struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>,\
struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>,\
    struct uvw::FsEvent<uvw::details::UVFsType::OPEN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::READ>,\
    struct uvw::FsEvent<uvw::details::UVFsType::WRITE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::STAT>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::UTIME>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>,\
    struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>,\
    struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>,\
    struct uvw::FsEvent<uvw::details::UVFsType::RENAME>,\
    struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::READLINK>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>,\
    struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>,\
    struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::READDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>,\
    struct uvw::FsEvent<uvw::details::UVFsType::STATFS>,\
    struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>,\
    struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>,\
    struct uvw::ErrorEvent>::Handler< EVENT >::on(class std::function<void( EVENT &, class uvw::FileReq&)>);


EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::ErrorEvent)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::OPEN>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::READ>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::WRITE>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::STAT>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::UTIME>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::RENAME>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LINK>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::READLINK>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::READDIR>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::STATFS>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>)
EXPLICIT_INSTANTIATION_FILE_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>)


template struct UVW_EXTERN uvw::Emitter<class uvw::FileReq, \
    struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>, \
    struct uvw::FsEvent<uvw::details::UVFsType::OPEN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READ>, \
    struct uvw::FsEvent<uvw::details::UVFsType::WRITE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::STAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::UTIME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>, \
    struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>, \
    struct uvw::FsEvent<uvw::details::UVFsType::RENAME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>, \
    struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::STATFS>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>, \
    struct uvw::ErrorEvent >::Connection<struct uvw::ErrorEvent>;

#define EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(EVENT) \
    template typename \
    uvw::Emitter<class uvw::FsReq, \
    struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>, \
    struct uvw::FsEvent<uvw::details::UVFsType::OPEN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READ>, \
    struct uvw::FsEvent<uvw::details::UVFsType::WRITE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::STAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::UTIME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>, \
    struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>, \
    struct uvw::FsEvent<uvw::details::UVFsType::RENAME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>, \
    struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::STATFS>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>, \
    struct uvw::ErrorEvent> \
    ::Handler<EVENT>::Connection \
    uvw::Emitter<class uvw::FsReq, \
    struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>, \
    struct uvw::FsEvent<uvw::details::UVFsType::OPEN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READ>, \
    struct uvw::FsEvent<uvw::details::UVFsType::WRITE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::STAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::UTIME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>, \
    struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>, \
    struct uvw::FsEvent<uvw::details::UVFsType::RENAME>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READLINK>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>, \
    struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>, \
    struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::READDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>, \
    struct uvw::FsEvent<uvw::details::UVFsType::STATFS>, \
    struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>, \
    struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>, \
    struct uvw::ErrorEvent> \
    ::Handler<EVENT> \
    ::on(class std::function<void(EVENT &,class uvw::FsReq &)>);

EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::ErrorEvent)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::UNKNOWN>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CUSTOM>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::OPEN>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CLOSE>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::READ>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::WRITE>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::SENDFILE>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::STAT>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LSTAT>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FSTAT>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FTRUNCATE>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::UTIME>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FUTIME>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::ACCESS>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CHMOD>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FCHMOD>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FSYNC>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FDATASYNC>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::UNLINK>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::RMDIR>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::MKDIR>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::MKDTEMP>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::RENAME>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::SCANDIR>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LINK>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::SYMLINK>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::READLINK>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CHOWN>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::FCHOWN>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::REALPATH>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::COPYFILE>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LCHOWN>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::OPENDIR>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::READDIR>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::CLOSEDIR>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::STATFS>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::MKSTEMP>)
EXPLICIT_INSTANTIATION_FS_REQ_EVENTS(struct uvw::FsEvent<uvw::details::UVFsType::LUTIME>)

template uvw::Flags<enum uvw::details::UVSymLinkFlags>::Flags(int);
template uvw::Flags<enum uvw::details::UVSymLinkFlags>::Flags(void);


template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::UNKNOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CUSTOM>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::OPEN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CLOSE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::READ>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::WRITE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::SENDFILE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::STAT>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LSTAT>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FSTAT>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FTRUNCATE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::UTIME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FUTIME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::ACCESS>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CHMOD>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FCHMOD>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FSYNC>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FDATASYNC>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::UNLINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::RMDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::MKDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::MKDTEMP>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::RENAME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::SCANDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::SYMLINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::READLINK>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CHOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::FCHOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::REALPATH>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::COPYFILE>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LCHOWN>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::OPENDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::READDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::CLOSEDIR>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::STATFS>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::MKSTEMP>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::FsEvent<details::UVFsType::LUTIME>>;
template struct UVW_EXTERN uvw::Emitter<class uvw::FsReq, struct uvw::FsEvent<details::UVFsType::UNKNOWN>, struct uvw::FsEvent<details::UVFsType::CUSTOM>, struct uvw::FsEvent<details::UVFsType::OPEN>, struct uvw::FsEvent<details::UVFsType::CLOSE>, struct uvw::FsEvent<details::UVFsType::READ>, struct uvw::FsEvent<details::UVFsType::WRITE>, struct uvw::FsEvent<details::UVFsType::SENDFILE>, struct uvw::FsEvent<details::UVFsType::STAT>, struct uvw::FsEvent<details::UVFsType::LSTAT>, struct uvw::FsEvent<details::UVFsType::FSTAT>, struct uvw::FsEvent<details::UVFsType::FTRUNCATE>, struct uvw::FsEvent<details::UVFsType::UTIME>, struct uvw::FsEvent<details::UVFsType::FUTIME>, struct uvw::FsEvent<details::UVFsType::ACCESS>, struct uvw::FsEvent<details::UVFsType::CHMOD>, struct uvw::FsEvent<details::UVFsType::FCHMOD>, struct uvw::FsEvent<details::UVFsType::FSYNC>, struct uvw::FsEvent<details::UVFsType::FDATASYNC>, struct uvw::FsEvent<details::UVFsType::UNLINK>, struct uvw::FsEvent<details::UVFsType::RMDIR>, struct uvw::FsEvent<details::UVFsType::MKDIR>, struct uvw::FsEvent<details::UVFsType::MKDTEMP>, struct uvw::FsEvent<details::UVFsType::RENAME>, struct uvw::FsEvent<details::UVFsType::SCANDIR>, struct uvw::FsEvent<details::UVFsType::LINK>, struct uvw::FsEvent<details::UVFsType::SYMLINK>, struct uvw::FsEvent<details::UVFsType::READLINK>, struct uvw::FsEvent<details::UVFsType::CHOWN>, struct uvw::FsEvent<details::UVFsType::FCHOWN>, struct uvw::FsEvent<details::UVFsType::REALPATH>, struct uvw::FsEvent<details::UVFsType::COPYFILE>, struct uvw::FsEvent<details::UVFsType::LCHOWN>, struct uvw::FsEvent<details::UVFsType::OPENDIR>, struct uvw::FsEvent<details::UVFsType::READDIR>, struct uvw::FsEvent<details::UVFsType::CLOSEDIR>, struct uvw::FsEvent<details::UVFsType::STATFS>, struct uvw::FsEvent<details::UVFsType::MKSTEMP>, struct uvw::FsEvent<details::UVFsType::LUTIME>, struct uvw::ErrorEvent>::Connection<struct uvw::ErrorEvent>;


}

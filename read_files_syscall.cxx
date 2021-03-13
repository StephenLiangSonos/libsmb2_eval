/// Sonos

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

#include <unistd.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <dirent.h>

using namespace std;

#define TMP_DIR "/tmp/tmp_mnt_1234"

int usage(void) {
    printf("dir_crawl_smb2 <URL> <username> <password> <vers>\n");
    exit(1);
}

string flatten(vector<string>& path) {
    string ret = "";
    for (int i = 0; i < path.size(); ++i) {
        ret += string(path[i]);
        ret += string("/");
    }
    if (!ret.empty()) {
        ret[ret.size() - 1] = '\0';
    }
    return ret;
}

void lsdir(string path, vector<string>& results) {
    DIR* dir = opendir(path.c_str());
    if (dir == NULL) {
        fprintf(stderr, "opendir %s failed\n", path.c_str());
        return;
    }

    struct dirent* ent;

    vector<string> nextdirs;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.' || ent->d_name[1] == '.') {
            continue;
        }

        switch (ent->d_type) {
            case DT_DIR:
                nextdirs.push_back(path + "/" + string(ent->d_name));
                break;
            case DT_REG:
                results.push_back(path + "/" + string(ent->d_name));
                break;
        }
    }

    for (string& nextdir : nextdirs) {
        lsdir(nextdir, results);
    }

    closedir(dir);
}

void readfile(string& file) {
    int fd = open(file.c_str(), O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "failed to open file %s\n", file.c_str());
        exit(2);
    }

    size_t bufsize = 4096;
    uint8_t buf[bufsize];

    int count = 0;
    int pos = 0;
    while ((count = read(fd, buf, bufsize)) > 0) {
        if (count == -1) {
            continue;
        }
        pos += count;
    }

    cout << "read " << pos << "B from " << file << endl;

    close(fd);
}

void readfiles(vector<string>& files) {
    for (string& file : files) {
        readfile(file);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        usage();
    }

    if (mkdir(TMP_DIR, O_RDWR) < 0) {
        fprintf(stderr, "mkdir %s failed: %s\n", TMP_DIR, strerror(errno));
        rmdir(TMP_DIR);
        exit(0);
    }

    char options[512];
    memset(options, 0, 512);
    sprintf(options, "vers=%s,user=%s,pass=%s,sec=ntlmssp", argv[4], argv[2], argv[3]);

    if (mount(argv[1], TMP_DIR, "cifs", O_RDONLY, options) != 0) {
        fprintf(stderr, "mount %s failed: %s\n", argv[1], strerror(errno));
        rmdir(TMP_DIR);
        exit(2);
    }

    vector<string> results;
    lsdir(TMP_DIR, results);

    readfiles(results);

    umount(TMP_DIR);
    rmdir(TMP_DIR);

    return 0;
}

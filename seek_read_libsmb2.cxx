/// Sonos

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <vector>

#include "smb2/smb2.h"
#include "smb2/libsmb2.h"

using namespace std;

int usage(void) {
    printf("dir_crawl_smb2 <URL> <username> <password> <vers>\n"
           "  format: "
           "smb://<host>/<share>/<path>"
           "\n");
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

void lsdir(struct smb2_context* ctx, string path, vector<string>& results) {
    struct smb2dir* dir = smb2_opendir(ctx, path.c_str());
    if (dir == NULL) {
        fprintf(stderr, "smb2_opendir failed to open %s", path.c_str());
        return;
    }

    vector<string> wip;
    struct smb2dirent *ent;
    while ((ent = smb2_readdir(ctx, dir))) {
        if (ent->name[0] == '.' || ent->name[1] == '.') { 
            continue;
        }

        switch (ent->st.smb2_type) {
            case SMB2_TYPE_LINK:
                break;
            case SMB2_TYPE_FILE:
                results.push_back(path + "/" + string(ent->name));
                break;
            case SMB2_TYPE_DIRECTORY:
                if (path.empty()) {
                    wip.push_back(string(ent->name));
                } else {
                    wip.push_back(path + "/" + string(ent->name));
                }
                break;
            default:
                break;
        }
    }
    smb2_closedir(ctx, dir);

    for (string& next_path : wip) {
        lsdir(ctx, next_path, results);
    }
}

void seek_and_read(struct smb2_context* ctx, struct smb2fh* fh, int new_pos) {
    size_t bufsize = 4096;
    uint8_t buf[bufsize];

    int pos = smb2_lseek(ctx, fh, new_pos, SEEK_SET, NULL);
    int count = smb2_pread(ctx, fh, buf, bufsize, pos);
    if (count < 0) {
        fprintf(stderr, "failed to read");
    }
}

void readfile(struct smb2_context* ctx, string& file) {
    struct smb2fh* fh = smb2_open(ctx, file.c_str(), O_RDONLY);
    if (fh == NULL) {
        fprintf(stderr, "smb2_open failed for %s", file.c_str());
        return;
    }


    struct smb2_stat_64 stat;
    if (smb2_fstat(ctx, fh, &stat) < 0) {
        fprintf(stderr, "fstat failed");
        smb2_close(ctx, fh);
        return;
    }

    seek_and_read(ctx, fh, stat.smb2_size / 5);
    seek_and_read(ctx, fh, stat.smb2_size / 4);
    seek_and_read(ctx, fh, stat.smb2_size / 3);
    seek_and_read(ctx, fh, stat.smb2_size / 2);

    cout << "read from " << file << endl;

    smb2_close(ctx, fh);
}

void readfiles(struct smb2_context* ctx, vector<string>& files) {
    for (string& file : files) {
        readfile(ctx, file);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        usage();
    }

    smb2_negotiate_version ver;
    if (argv[4][0] == '2') {
        ver = SMB2_VERSION_ANY2;
    } else if (argv[4][0] == '3') {
        ver = SMB2_VERSION_ANY3;
    } else {
            usage();
    }

    struct smb2_context* smb2 = smb2_init_context();
    if (smb2 == NULL) {
        fprintf(stderr, "smb2_init_context failed");
        exit(0);
    }

    struct smb2_url* url = smb2_parse_url(smb2, argv[1]);
    if (url == NULL) {
        fprintf(stderr, "smb2_parse_url failed");
        exit(0);
    }

    smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);

    smb2_set_user(smb2, argv[2]);
    smb2_set_password(smb2, argv[3]);
    smb2_set_version(smb2, ver);

    if (smb2_connect_share(smb2, url->server, url->share, url->user) < 0) {
        fprintf(stderr, "smb2_connect_share failed");
        exit(10);
    }

    vector<string> result;
    lsdir(smb2, url->path ? url->path : "", result);

    readfiles(smb2, result);

    smb2_disconnect_share(smb2);
    smb2_destroy_url(url);
    smb2_destroy_context(smb2);
    return 0;
}

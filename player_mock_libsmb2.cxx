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
    printf("dir_crawl_smb2 <URL> <username> <password> <num_runs>\n"
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

void seek_and_read(struct smb2_context* ctx, struct smb2fh* fh, int new_pos) {
    size_t bufsize = 4096;
    uint8_t buf[bufsize];

    int pos = smb2_lseek(ctx, fh, new_pos, SEEK_SET, NULL);
    int count = smb2_read(ctx, fh, buf, bufsize);
    if (count < 0) {
        fprintf(stderr, "failed to read");
    }
}

void lsdir(struct smb2_context* ctx, string path, vector<string>& results) {
    struct smb2dir* dir = smb2_lazy_opendir(ctx, path.c_str());
    if (dir == NULL) {
        fprintf(stderr, "error: %s\n", smb2_get_error(ctx));
        return;
    }

    vector<string> wip;
    struct smb2dirent *ent;
    while ((ent = smb2_lazy_readdir(ctx, dir))) {
        if (ent->name[0] == '.' || ent->name[1] == '.') { 
            continue;
        }

        string cur_ent;
        switch (ent->st.smb2_type) {
            case SMB2_TYPE_LINK:
                break;
            case SMB2_TYPE_FILE:
                cur_ent = path + "/" + string(ent->name);
                results.push_back(cur_ent);
                break;
            case SMB2_TYPE_DIRECTORY:
                if (path.empty()) {
                    cur_ent = string(ent->name);
                } else {
                    cur_ent = path + "/" + string(ent->name);
                }
                wip.push_back(cur_ent);
                break;
            default:
                break;
        }

        struct smb2_stat_64 stat;
        printf("stat %s\n", cur_ent.c_str());
        if (smb2_stat(ctx, cur_ent.c_str(), &stat) < 0) {
            printf("stat %s failed: %s\n", cur_ent.c_str(), smb2_get_error(ctx));
        } else if(SMB2_TYPE_FILE == ent->st.smb2_type) {
            struct smb2fh* fh = smb2_open(ctx, cur_ent.c_str(), O_RDONLY);
            if (fh) {
                seek_and_read(ctx, fh, stat.smb2_size / 5);
                smb2_close(ctx, fh);
            } else {
                fprintf(stderr, "smb2_open failed for %s\n", cur_ent.c_str());
            }
        }
    }
    smb2_closedir(ctx, dir);

    for (string& next_path : wip) {
        lsdir(ctx, next_path, results);
    }
}

int run(int argc, char *argv[]) {
    if (argc < 5) {
        usage();
    }

    smb2_negotiate_version ver;
    //ver = SMB2_VERSION_ANY2;
    ver = SMB2_VERSION_ANY3;

    struct smb2_context* smb2 = smb2_init_context();
    if (smb2 == NULL) {
        fprintf(stderr, "smb2_init_context failed");
        return 0;
    }

    struct smb2_url* url = smb2_parse_url(smb2, argv[1]);
    if (url == NULL) {
        fprintf(stderr, "smb2_parse_url failed");
        fprintf(stderr, "error: %s\n", smb2_get_error(smb2));
        return 0;
    }

    smb2_set_security_mode(smb2, SMB2_NEGOTIATE_SIGNING_ENABLED);

    smb2_set_user(smb2, argv[2]);
    smb2_set_password(smb2, argv[3]);
    smb2_set_version(smb2, ver);

    if (smb2_connect_share(smb2, url->server, url->share, url->user) < 0) {
        fprintf(stderr, "smb2_connect_share failed");
        fprintf(stderr, "error: %s\n", smb2_get_error(smb2));
        return 0;
    }

    vector<string> result;
    lsdir(smb2, url->path ? url->path : "", result);

    cout << "found " << result.size() << " files\n";

    smb2_disconnect_share(smb2);
    smb2_destroy_url(url);
    smb2_destroy_context(smb2);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        usage();
    }

    unsigned long num_runs = std::stoul(string(argv[4]));
    for (unsigned i = 0; i < num_runs; ++i) {
        printf("############ run %u ###############\n", i);
        run(argc, argv);
    }
}

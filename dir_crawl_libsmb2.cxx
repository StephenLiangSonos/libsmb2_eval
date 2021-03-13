/// Sonos

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <vector>

#include "smb2/smb2.h"
#include "smb2/libsmb2.h"

using namespace std;

int usage(void) {
    printf("dir_crawl_smb2 <URL> <username> <password>\n"
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

int main(int argc, char *argv[]) {
    if (argc < 4) {
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

    if (smb2_connect_share(smb2, url->server, url->share, url->user) < 0) {
        fprintf(stderr, "smb2_connect_share failed");
        exit(10);
    }

    vector<string> result;
    lsdir(smb2, url->path ? url->path : "", result);

    cout << "found " << result.size() << " files\n";

    smb2_disconnect_share(smb2);
    smb2_destroy_url(url);
    smb2_destroy_context(smb2);
    return 0;
}

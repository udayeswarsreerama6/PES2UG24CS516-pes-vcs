// tree.c — Tree object serialization and construction
//
// PROVIDED functions: get_file_mode, tree_parse, tree_serialize
// IMPLEMENTED functions: tree_from_index
//
// Binary tree format (per entry, concatenated with no separators):
//   "<mode-as-ascii-octal> <name>\0<32-byte-binary-hash>"

#include "tree.h"
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Forward declarations (implemented in object.c)
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ─── Mode Constants ──────────────────────────────────────────────────────────
#define MODE_FILE 0100644
#define MODE_EXEC 0100755
#define MODE_DIR  0040000

// ─── PROVIDED ────────────────────────────────────────────────────────────────

uint32_t get_file_mode(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;
    if (S_ISDIR(st.st_mode)) return MODE_DIR;
    if (st.st_mode & S_IXUSR) return MODE_EXEC;
    return MODE_FILE;
}

int tree_parse(const void *data, size_t len, Tree *tree_out) {
    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *entry = &tree_out->entries[tree_out->count];

        const uint8_t *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1;

        char mode_str[16] = {0};
        size_t mode_len = space - ptr;
        if (mode_len >= sizeof(mode_str)) return -1;
        memcpy(mode_str, ptr, mode_len);
        entry->mode = strtol(mode_str, NULL, 8);
        ptr = space + 1;

        const uint8_t *null_byte = memchr(ptr, '\0', end - ptr);
        if (!null_byte) return -1;

        size_t name_len = null_byte - ptr;
        if (name_len >= sizeof(entry->name)) return -1;
        memcpy(entry->name, ptr, name_len);
        entry->name[name_len] = '\0';
        ptr = null_byte + 1;

        if (ptr + HASH_SIZE > end) return -1;
        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }

    return 0;
}

static int compare_tree_entries(const void *a, const void *b) {
    return strcmp(((const TreeEntry *)a)->name, ((const TreeEntry *)b)->name);
}

int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    size_t max_size = (size_t)tree->count * 296;
    uint8_t *buffer = malloc(max_size);
    if (!buffer) return -1;

    Tree sorted_tree = *tree;
    qsort(sorted_tree.entries, (size_t)sorted_tree.count, sizeof(TreeEntry), compare_tree_entries);

    size_t offset = 0;
    for (int i = 0; i < sorted_tree.count; i++) {
        const TreeEntry *entry = &sorted_tree.entries[i];
        int written = sprintf((char *)buffer + offset, "%o %s", entry->mode, entry->name);
        offset += written + 1; // +1 for the null terminator
        memcpy(buffer + offset, entry->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out = offset;
    return 0;
}

// ─── IMPLEMENTED ──────────────────────────────────────────────────────────────

// Recursive helper: given a slice of index entries all sharing the same
// path prefix (at depth `depth`), build and write a tree object for that level.
// Returns 0 on success, -1 on error.
static int write_tree_level(IndexEntry *entries, int count, int depth, ObjectID *id_out) {
    Tree tree;
    tree.count = 0;

    int i = 0;
    while (i < count) {
        // Find the component at this depth by scanning path for '/'
        const char *full_path = entries[i].path;

        // Walk past 'depth' slashes to find the component at this depth
        const char *component = full_path;
        for (int d = 0; d < depth; d++) {
            const char *slash = strchr(component, '/');
            if (!slash) {
                // Fewer components than depth — shouldn't happen
                return -1;
            }
            component = slash + 1;
        }

        // Check if there's another slash after this component (meaning it's a subtree)
        const char *next_slash = strchr(component, '/');

        if (next_slash == NULL) {
            // This is a leaf (direct file at this level)
            // Entry name is `component` itself
            TreeEntry *te = &tree.entries[tree.count];
            strncpy(te->name, component, sizeof(te->name) - 1);
            te->name[sizeof(te->name) - 1] = '\0';
            te->mode = entries[i].mode;
            te->hash = entries[i].hash;
            tree.count++;
            i++;
        } else {
            // This is a directory component — collect all entries in this subtree
            // The directory name is the substring up to next_slash
            size_t dir_name_len = (size_t)(next_slash - component);
            char dir_name[256];
            if (dir_name_len >= sizeof(dir_name)) return -1;
            memcpy(dir_name, component, dir_name_len);
            dir_name[dir_name_len] = '\0';

            // Find all entries sharing this same directory name at this depth
            int j = i;
            while (j < count) {
                const char *cp = entries[j].path;
                for (int d = 0; d < depth; d++) {
                    cp = strchr(cp, '/') + 1;
                }
                const char *ns = strchr(cp, '/');
                if (ns == NULL) break; // leaf — different level
                size_t dnl = (size_t)(ns - cp);
                if (dnl != dir_name_len || strncmp(cp, dir_name, dir_name_len) != 0) break;
                j++;
            }

            // Recursively write the subtree for entries[i..j)
            ObjectID subtree_id;
            if (write_tree_level(entries + i, j - i, depth + 1, &subtree_id) != 0)
                return -1;

            TreeEntry *te = &tree.entries[tree.count];
            strncpy(te->name, dir_name, sizeof(te->name) - 1);
            te->name[sizeof(te->name) - 1] = '\0';
            te->mode = MODE_DIR;
            te->hash = subtree_id;
            tree.count++;

            i = j;
        }
    }

    // Serialize and write the tree object
    void *tree_data;
    size_t tree_len;
    if (tree_serialize(&tree, &tree_data, &tree_len) != 0) return -1;

    int rc = object_write(OBJ_TREE, tree_data, tree_len, id_out);
    free(tree_data);
    return rc;
}

// Build a tree hierarchy from the current index and write all tree
// objects to the object store. Returns root tree ObjectID in *id_out.
int tree_from_index(ObjectID *id_out) {
    Index index;
    if (index_load(&index) != 0) return -1;
    if (index.count == 0) {
        // Empty tree: write an empty tree object
        Tree empty_tree;
        empty_tree.count = 0;
        void *tree_data;
        size_t tree_len;
        if (tree_serialize(&empty_tree, &tree_data, &tree_len) != 0) return -1;
        int rc = object_write(OBJ_TREE, tree_data, tree_len, id_out);
        free(tree_data);
        return rc;
    }

    return write_tree_level(index.entries, index.count, 0, id_out);
}

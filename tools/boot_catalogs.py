def boot_pg_type(pg_type_h, boot_c, boot_h):
    Natts_pg_type = 30
    Anum_pg_type_typname = 1
    Anum_pg_type_typnamespace = 2
    Anum_pg_type_typowner = 3
    Anum_pg_type_typlen = 4
    Anum_pg_type_typbyval = 5
    Anum_pg_type_typtype = 6
    Anum_pg_type_typcategory = 7
    Anum_pg_type_typispreferred = 8
    Anum_pg_type_typisdefined = 9
    Anum_pg_type_typdelim = 10
    Anum_pg_type_typrelid = 11
    Anum_pg_type_typelem = 12
    Anum_pg_type_typarray = 13
    Anum_pg_type_typinput = 14
    Anum_pg_type_typoutput = 15
    Anum_pg_type_typreceive = 16
    Anum_pg_type_typsend = 17
    Anum_pg_type_typmodin = 18
    Anum_pg_type_typmodout = 19
    Anum_pg_type_typanalyze = 20
    Anum_pg_type_typalign = 21
    Anum_pg_type_typstorage = 22
    Anum_pg_type_typnotnull = 23
    Anum_pg_type_typbasetype = 24
    Anum_pg_type_typtypmod = 25
    Anum_pg_type_typndims = 26
    Anum_pg_type_typcollation = 27
    Anum_pg_type_typdefaultbin = 28
    Anum_pg_type_typdefault = 29
    Anum_pg_type_typacl = 30

    oid_offset = 3
    attr_offset = 5
    print('#pragma once\n', file=boot_h)
    print('#include "postgres.h"', file=boot_h)
    print('#include "catalog/pg_authid.h"', file=boot_h)
    print('#include "catalog/pg_namespace.h"', file=boot_h)
    print('#include "catalog/pg_type.h"', file=boot_h)
    print('#include "utils/fmgroids.h"', file=boot_h)
    print('', file=boot_h)
    print("struct gpc_boot_pg_type_data { Oid oid; FormData_pg_type data; };\n", file=boot_h)
    print("extern const struct gpc_boot_pg_type_data gpc_boot_pg_type[];\n", file=boot_h)
    print(f'extern const int gpc_boot_pg_type_num_rows;', file=boot_h)

    print('#include "boot.h"', file=boot_c)
    print('', file=boot_c)
    print("const struct gpc_boot_pg_type_data gpc_boot_pg_type[] = {", file=boot_c)
    num = 0
    for line in pg_type_h:
        if line[0:4] == 'DATA':
            tokens = line.split()
            oid = tokens[oid_offset]
            for attr_num in range(1, Natts_pg_type + 1):
                attr_idx = attr_offset + attr_num - 1
                if tokens[attr_idx] == '_null_':
                    tokens[attr_idx] = 'NULL'
                elif tokens[attr_idx] == '-':
                    tokens[attr_idx] = '0'
                elif tokens[attr_idx] == 't':
                    tokens[attr_idx] = 'true'
                elif tokens[attr_idx] == 'f':
                    tokens[attr_idx] = 'false'
                elif tokens[attr_idx] == 'PGNSP':
                    tokens[attr_idx] = 'PG_CATALOG_NAMESPACE'
                elif tokens[attr_idx] == 'PGUID':
                    tokens[attr_idx] = 'BOOTSTRAP_SUPERUSERID'
                elif tokens[attr_idx] == 'NAMEDATALEN':
                    pass
                elif tokens[attr_idx] == 'FLOAT4PASSBYVAL':
                    pass
                elif tokens[attr_idx] == 'FLOAT8PASSBYVAL':
                    pass
                elif tokens[attr_idx] == "SIZEOF_POINTER":
                    tokens[attr_idx] = "sizeof(Pointer)"
                elif tokens[attr_idx] == "ALIGNOF_POINTER":
                    tokens[attr_idx] = "((sizeof(Pointer) == 4) ? 'i' : 'd')"
                elif Anum_pg_type_typinput <= attr_num <= Anum_pg_type_typanalyze:
                    tokens[attr_idx] = "F_" + tokens[attr_idx].upper()
                elif not tokens[attr_idx].isdigit() and tokens[attr_idx] != '-1' and tokens[attr_idx] != '-2':
                    if len(tokens[attr_idx]) == 1 or tokens[attr_idx][0] == "\\":
                        tokens[attr_idx] = "'" + tokens[attr_idx] + "'"
                    else:
                        tokens[attr_idx] = '"' + tokens[attr_idx] + '"'
                        if attr_num == Anum_pg_type_typname:
                            tokens[attr_idx] = '{ ' + tokens[attr_idx] + ' }'
            attr_low = attr_offset + Anum_pg_type_typname - 1
            attr_high = attr_offset + Anum_pg_type_typdefaultbin - 1
            print('\t{ %s, { %s } },' %
                    (oid, ", ".join(tokens[attr_low:attr_high])), file=boot_c)
            num += 1
    print("};\n", file=boot_c)
    print(f'const int gpc_boot_pg_type_num_rows = {num};', file=boot_c)
    pass


if __name__ == "__main__":
    pg_type_h = open("../lib/gpdb/src/include/catalog/pg_type.h", 'r')
    boot_c = open("../src/boot.c", 'w+')
    boot_h = open("../src/boot.h", "w+")

    boot_pg_type(pg_type_h, boot_c, boot_h)
    
    pg_type_h.close()
    boot_c.close()
    boot_h.close()

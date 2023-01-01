# vettel

A simple key-value store written in ~400 lines of C code. currently, you can perform the following commands.

```bash
$ vt
>>> list
0) "this_is_the_key"
1) "key"
>>> get key
this is the value
>>> set  new_value
a record with that name already exists
"key" : "this is the value"
```

# building

```bash
$ ./make.sh # will output the binary at ./out/vt
$ ./out/vt
```

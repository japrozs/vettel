# vettel

A simple key-value store written in ~400 lines of C code. currently, you can perform the following commands.

```bash
$ vt
>>> list
0) "this_is_the_key"
1) "key"
>>> get key
value
>>> get this_is_the_key
this is the value
>>> set key new_value
error: a record with key "key" already exists
>>> del key
>>> list
0) "this_is_the_key"
>>> set key new_value
>>> q
```

# building

```bash
$ ./make.sh # will output the binary at ./out/vt
$ ./out/vt
```

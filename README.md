# Riemersma dither

See https://www.compuphase.com/riemer.htm

This program uses [Gilbert Curve](https://github.com/jakubcerveny/gilbert).

# compile

Requires NetPBM.

```
chpl --fast ppmriemer.chpl 
```

# usage

```
ppmriemer --ifile=input.ppm --pfile=palette.ppm --ofile=output.ppm 
```

`palette` file should be produced by `pnmcolormap`.

# LICENSE

BSD 2-Clause "Simplified" License

# Example

![original](example/in.png)


Use a [8 color palette](example/p.ppm).

![output](example/out.png)

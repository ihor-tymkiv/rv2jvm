_start:
	addi x5, x0, 0
	addi x6, x0, 5
loop:
	addi x5, x5, 1
	add x5, x5, x5
	blt x5, x6, loop
	sb x5, 100(x0)
	j end
end:

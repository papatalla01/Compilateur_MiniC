
.data


.text

main:
    addiu $29, $29, -8
    addiu $9, $0, 1
    sw    $9, 4($29)
    addiu $9, $0, 2
    sw    $9, 0($29)
    addiu $29, $29, 8
    addiu $2, $0, 10
    syscall

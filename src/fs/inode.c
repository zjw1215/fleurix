#include <param.h>
#include <x86.h>
#include <proto.h>
#include <proc.h>
// 
#include <buf.h>
#include <conf.h>
//
#include <super.h>
#include <inode.h>
#include <fs.h>

struct inode inode[NINODE];

/*
 * this file indicated some operation on inode. such as
 * iget -> get an locked inode.
 * iput -> release an locked inode.
 *
 * */

/* get an (locked) inode via an number 
 * if the inode is in cache, return it right now.
 * return NULL on error.
 *
 * note1: you may compare this code with getblk, the idea here
 * is common used among everywhere on resource allocation.
 * it returns ONLY locked inode just as B_BUSY in getblk, just 
 * to prevent other processes' accessing within one syscall.
 *
 * note2: though there is still some differences between getblk,
 * put an eye on *reference count*, lock won't stays long(within 
 * one syscall, like the open routine, which unlock the inode at
 * last), though *reference count* ramains set between syscalls
 * to prevent the kernel from reallocating active in-core inode.
 *
 * */
struct inode* iget(ushort dev, uint num){
    struct buf *bp;
    struct inode *ip;
    struct super *sp;

_loop:
    for(ip=&inode[0]; ip<&inode[NINODE]; ip++){
        if (ip->i_dev==dev && ip->i_num==num) {
            // if found but locked.
            if (ip->i_flag & I_LOCK) {
                ip->i_flag |= I_WANTED;
                sleep(ip, PINOD);
                goto _loop;
            }
            // if this is an mount point, redirect to the mount
            // target's root inode.
            if (ip->i_flag & I_MOUNT) {
                for (sp=&mnt[0]; sp<&mnt[NMOUNT]; sp++){
                    if (ip == sp->s_imnt){
                        // iget(sp->s_dev, ROOTINO);
                        dev = sp->s_dev;
                        num = ROOTINO;
                        goto _loop;
                    }
                }
            }
            // if found straightly
            ip->i_count++;
            ip->i_flag |= I_LOCK;
            return ip;
        }
    }
    // not caced, so seek one free slot
    for(ip=&inode[0]; ip<&inode[NINODE]; ip++){
        // time to read disk
        if (ip->i_count==0) {
            ip->i_dev = dev;
            ip->i_num = num;
            ip->i_flag = I_LOCK;
            ip->i_count++;
            read_inode(ip);
            return ip;
        }
    }
    // if not found (no free slot) 
    printf("inode table overflow.\n");
    return NULL;
}

/* decrease the reference count, write updates to disk if
 * nessary.
 * release an inode */
void iput(struct inode *ip){
    ip->i_flag |= I_LOCK;
    if (ip->i_count==1){
        write_inode(ip);
        ip->i_flag = 0;
        ip->i_num = 0;
    }
    ip->i_count--;
    unlock_inode(ip);
}

/***************************************************/

/*
 * Given an inode and a position within the corresponding file, locate the
 * block (not zone) number in which that position is to be found and return it.
 * returns 0 on error.
 *
 * note: 
 * the first 7 entry of ip->zones[] are direct pointers, ip->zone[7] is an indirect 
 * pointer to a zone map, while ip->zone[8] is an double indirect pointer to a zone map.
 * TODO: debug it!.
 */
int bmap(struct inode *ip, ushort nr) {
    struct buf *bp, *bp2;
    short *zmap, *zmap2;
    ushort ret;

    if ((nr>7+512+512*512) || (nr > (ip->i_size / BLK))) {
        panic("blk nr too big.");
    }
    if (nr<7){
        return ip->i_zone[nr];
    }
    nr -= 7;
    // read the indirect zone map
    if (nr<512){
        if (ip->i_zone[7]==0) return 0;
        bp = bread(ip->i_dev, ip->i_zone[7]);
        zmap = (short *)bp->b_data;
        ret = zmap[nr];
        brelse(bp);
        return ret;
    }
    nr -= 512;
    // the double indirect zone map.
    // read the middle indirect zone map.
    if (ip->i_zone[8]==0) return 0;
    bp = bread(ip->i_dev, ip->i_zone[8]);
    zmap = (short *)bp->b_data;
    if (zmap[nr/512]==NULL) return 0;
    // read the secondary indirect zone map.
    bp2 = bread(ip->i_dev, zmap[nr/512]);
    zmap2 = (short*)bp2->b_data;
    ret = zmap2[nr%512];
    brelse(bp);
    brelse(bp2);
    return ret;
}

/* 
 * TODO: similar with bmap, but used on writing file. 
 * allocate one block and map it onto file's logical block number, if 
 * nessary.
 * returns the device's physical block number.
 * */
int put_blk(struct inode *ip, ushort nr){
}

/***************************************************/

/* read/write a inode from disk 
 * */
int read_inode(struct inode *ip){
    struct super *sp;
    struct d_inode *itab; /* note this is an d_inode, 32 bytes. */
    struct buf *bp;
    uint lba;

    sp = get_super(ip->i_dev);
    if (sp==NULL){
        panic("error on reading a super");
    }
    // get the blk number where this inode lies in.
    lba = 2 + (sp->s_nimap_blk) + (sp->s_nzmap_blk) + (ip->i_num-1)/NINO_PER_BLK;
    bp = bread(ip->i_dev, lba);
    if (bp->b_flag & B_ERROR) {
        panic("error on reading an inode");
    }
    itab = (struct d_inode*)bp->b_data;
    memcpy(ip, &itab[(ip->i_num-1)%NINO_PER_BLK], sizeof(struct d_inode));
    brelse(bp);
    return 0;
}

/* TODO: fulfill it */
void write_inode(struct inode *ip){
}

/*************************************************************/

/* remember this just free with malloc. */
void unlock_inode(struct inode *ip){
    if (ip->i_flag & I_WANTED) {
        wakeup(ip);
    }
    ip->i_flag &= ~(I_LOCK | I_WANTED);
}

/*************************************************************/

void dump_inode(struct inode *ip){
    printf("i_mode:%x\n", ip->i_mode);
    printf("i_uid:%x\n", ip->i_uid);
    printf("i_size:%x\n", ip->i_size);
    printf("i_time:%x\n", ip->i_time);
    printf("i_gid:%x\n", ip->i_gid);
    printf("i_nlinks:%x\n", ip->i_nlinks);
    printf("i_count:%x\n", ip->i_count);
    printf("i_dev:%x\n", ip->i_dev);
    printf("i_num:%x\n", ip->i_num);
    printf("i_flag:%x\n", ip->i_flag);
    int i=0;
    for(i=0;i<9;i++){
        printf("i_zone[%d]: %x\n", i, ip->i_zone[i]);
    }

    /*
    uint nr;
    struct buf *bp;

    nr = bmap(ip, 0);
    printf("bmap: %d\n", nr);
    bp = bread(rootdev, nr);
    dump_buf(bp);
    brelse(bp);
    printf("\n"); */
}
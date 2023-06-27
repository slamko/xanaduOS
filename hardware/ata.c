
enum {
    ATA_ST_BUSY       = (1 << 7),
    ATA_ST_READY      = (1 << 6),
    ATA_ST_WR_FAULT   = (1 << 5),
    ATA_ST_SEEK_CMPL  = (1 << 4),
    ATA_ST_RQ_READY   = (1 << 3),
    ATA_ST_CORRECT    = (1 << 2),
    ATA_ST_ID         = (1 << 1),
    ATA_ST_ERR        = (1 << 0),
};



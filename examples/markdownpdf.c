#include "markdown.h"
#include "html.h"
#include "buffer.h"
#include "pdfreport.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_UNIT 1024
#define OUTPUT_UNIT 64

static void
mdpdf_header(struct buf *ob, const struct buf *text, int level, void *opaque)
{
    char tmp = 0;
    HPDF_HANDLER *hpdf = opaque;
    printf("\n\nlevel:%d, %d, %s\n\n", level, text->size, text->data);
    if(text->size > 0) {
        switch(level) {
        case 1:
            HPDF_CSS_SET(PDF_CSS_TYPE_H1, &((*hpdf)->css));
            break;
        case 2:
            HPDF_CSS_SET(PDF_CSS_TYPE_H2, &((*hpdf)->css));
            break;
        case 3:
            HPDF_CSS_SET(PDF_CSS_TYPE_H3, &((*hpdf)->css));
            break;
        case 4:
            HPDF_CSS_SET(PDF_CSS_TYPE_H4, &((*hpdf)->css));
            break;
        case 5:
            HPDF_CSS_SET(PDF_CSS_TYPE_H5, &((*hpdf)->css));
            break;
        case 6:
            HPDF_CSS_SET(PDF_CSS_TYPE_H6, &((*hpdf)->css));
            break;
        default:
            printf("header level error:%d\n", level);
            HPDF_CSS_SET(PDF_CSS_TYPE_P, &((*hpdf)->css));
            break;
        }
        tmp = ((char *)text->data)[text->size];
        ((char *)text->data)[text->size] = 0;
        printf("header=============%s\n", text->data);
        HPDF_HANDLER_PrintEx(*hpdf, (char *)text->data);
        ((char *)text->data)[text->size] = tmp;
    } else {
        printf("\n\ntext:%d\n\n", text->size);
    }
}

static void
mdpdf_paragraph(struct buf *ob, const struct buf *text, void *opaque)
{
    char tmp = 0;
    HPDF_HANDLER *hpdf = opaque;
    if (text->size > 0) {
        tmp = ((char *)text->data)[text->size];
        ((char *)text->data)[text->size] = 0;
        printf("p=============%s\n", text->data);
        HPDF_CSS_SET(PDF_CSS_TYPE_P, &((*hpdf)->css));
        HPDF_HANDLER_PrintEx(*hpdf, (char *)text->data);
        ((char *)text->data)[text->size] = tmp;
    } else {
        printf("\n\ntext:%d\n\n", text->size);
    }
}

static void
mdpdf_blockpre(struct buf *ob, const struct buf *text, const struct buf *lang, void *opaque)
{
    char tmp = 0;
    HPDF_HANDLER *hpdf = opaque;
    if (text->size > 0) {
        tmp = ((char *)text->data)[text->size];
        ((char *)text->data)[text->size] = 0;
        printf("pre=============%s\n", text->data);
        HPDF_CSS_SET(PDF_CSS_TYPE_PRE, &((*hpdf)->css));
        HPDF_HANDLER_PrintEx(*hpdf, (char *)text->data);
        ((char *)text->data)[text->size] = tmp;
    } else {
        printf("\n\ntext:%d\n\n", text->size);
    }
}

void
sdpdf_md_renderer(struct sd_callbacks *callbacks, HPDF_HANDLER *options)
{
    static const struct sd_callbacks cb_default = {
        mdpdf_blockpre,
        NULL,
        NULL,
        mdpdf_header,
        NULL,
        NULL,
        NULL,
        mdpdf_paragraph,
        NULL,
        NULL,
        NULL,

        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,

        NULL,
        NULL,

        NULL,
        NULL,
    };
    
    *options = HPDF_HANDLER_New();
    if(!(*options)) {
        printf("hpdf handle new failed!\n");
        return;   
    }
    HPDF_CSS_SET(PDF_CSS_TYPE_P, &((*options)->css));

    memcpy(callbacks, &cb_default, sizeof(struct sd_callbacks));
}

/* main • main function, interfacing STDIO with the parser */
int
main(int argc, char **argv)
{
    struct buf *ib, *ob;
    int ret;
    FILE *in = stdin;

    struct sd_callbacks callbacks;
    struct sd_markdown *markdown;
    HPDF_HANDLER hpdf;

    const char *fname = "2.pdf";

    /* opening the file if given from the command line */
    if (argc > 1) {
        in = fopen(argv[1], "r");
        if (!in) {
            fprintf(stderr,"Unable to open input file \"%s\": %s\n", argv[1], strerror(errno));
            return 1;
        }
    }

    /* reading everything */
    ib = bufnew(READ_UNIT);
    bufgrow(ib, READ_UNIT);
    while ((ret = fread(ib->data + ib->size, 1, ib->asize - ib->size, in)) > 0) {
        ib->size += ret;
        bufgrow(ib, ib->size + READ_UNIT);
    }

    if (in != stdin)
        fclose(in);

    /* performing markdown parsing */
    ob = bufnew(OUTPUT_UNIT);

    sdpdf_md_renderer(&callbacks, &hpdf);
    markdown = sd_markdown_new(0, 16, &callbacks, &hpdf);

    sd_markdown_render(ob, ib->data, ib->size, markdown);
    sd_markdown_free(markdown);

    /* writing the result to stdout */
    //ret = fwrite(ob->data, 1, ob->size, stdout);

    /* cleanup */
    bufrelease(ib);
    bufrelease(ob);

    /* save the document to a file */
    HPDF_SaveToFile (hpdf->pdf, fname);

    /* clean up */
    HPDF_HANDLER_Free(hpdf);

    return (ret < 0) ? -1 : 0;
}

/* vim: set filetype=c: */

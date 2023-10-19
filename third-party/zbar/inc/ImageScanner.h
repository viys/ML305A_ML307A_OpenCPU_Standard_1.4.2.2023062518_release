#ifndef _ZBAR_IMAGE_SCANNER_H_
#define _ZBAR_IMAGE_SCANNER_H_

/// @file
/// Image Scanner C++ wrapper

#ifndef _ZBAR_H_
# error "include zbar.h in your application, **not** zbar/ImageScanner.h"
#endif

#include "Image.h"

namespace zbar {

/// mid-level image scanner interface.
/// reads barcodes from a 2-D Image

class ImageScanner {
public:
    /// constructor.
    ImageScanner (zbar_image_scanner_t *scanner = NULL)
    {
        if(scanner)
            _scanner = scanner;
        else
            _scanner = zbar_image_scanner_create();
    }

    ~ImageScanner ()
    {
        zbar_image_scanner_destroy(_scanner);
    }

    /// cast to C image_scanner object
    operator zbar_image_scanner_t* () const
    {
        return(_scanner);
    }

    /// setup result handler callback.
    void set_handler (Image::Handler &handler)
    {
        zbar_image_scanner_set_data_handler(_scanner, handler, &handler);
    }

    /// set config for indicated symbology (0 for all) to specified value.
    /// @see zbar_image_scanner_set_config()
    /// @since 0.4
    int set_config (zbar_symbol_type_t symbology,
                    zbar_config_t config,
                    int value)
    {
        return(zbar_image_scanner_set_config(_scanner, symbology,
                                              config, value));
    }

    /// set config parsed from configuration string.
    /// @see zbar_image_scanner_parse_config()
    /// @since 0.4
    int set_config (std::string cfgstr)
    {
        return(zbar_image_scanner_parse_config(_scanner, cfgstr.c_str()));
    }

    /// enable or disable the inter-image result cache.
    /// see zbar_image_scanner_enable_cache()
    void enable_cache (bool enable = true)
    {
        zbar_image_scanner_enable_cache(_scanner, enable);
    }

    /// remove previous results from scanner and image.
    /// @see zbar_image_scanner_recycle_image()
    /// @since 0.10
    void recycle_image (Image &image)
    {
        zbar_image_scanner_recycle_image(_scanner, image);
    }

    /// retrieve decode results for last scanned image.
    /// @see zbar_image_scanner_get_results()
    /// @since 0.10
    const SymbolSet get_results () const {
        return(SymbolSet(zbar_image_scanner_get_results(_scanner)));
    }

    /// scan for symbols in provided image.
    /// see zbar_scan_image()
    int scan (Image& image)
    {
        return(zbar_scan_image(_scanner, image));
    }

    /// scan for symbols in provided image.
    /// see zbar_scan_image()
    ImageScanner& operator<< (Image& image)
    {
        scan(image);
        return(*this);
    }

private:
    zbar_image_scanner_t *_scanner;
};

}

#endif

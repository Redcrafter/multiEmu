#include <AppKit/AppKit.h>
#include "nfd.h"

static NSArray *BuildAllowedFileTypes( const char *filterList ) {
    // Commas and semicolons are the same thing on this platform

    NSMutableArray *buildFilterList = [[NSMutableArray alloc] init];

    char typebuf[NFD_MAX_STRLEN] = {0};
    
    size_t filterListLen = strlen(filterList);
    char *p_typebuf = typebuf;
    for ( size_t i = 0; i < filterListLen+1; ++i ) {
        if ( filterList[i] == ',' || filterList[i] == ';' || filterList[i] == '\0' ) {
            if (filterList[i] != '\0')
                ++p_typebuf;
            *p_typebuf = '\0';

            NSString *thisType = [NSString stringWithUTF8String: typebuf];
            [buildFilterList addObject:thisType];
            p_typebuf = typebuf;
            *p_typebuf = '\0';
        } else {
            *p_typebuf = filterList[i];
            ++p_typebuf;

        }
    }

    NSArray *returnArray = [NSArray arrayWithArray:buildFilterList];

    [buildFilterList release];
    return returnArray;
}

static void AddFilterListToDialog( NSSavePanel *dialog, const char *filterList ) {
    if ( !filterList || strlen(filterList) == 0 )
        return;

    NSArray *allowedFileTypes = BuildAllowedFileTypes( filterList );
    if ( [allowedFileTypes count] != 0 ) {
        [dialog setAllowedFileTypes:allowedFileTypes];
    }
}

static void SetDefaultPath( NSSavePanel *dialog, const char *defaultPath ) {
    if ( !defaultPath || strlen(defaultPath) == 0 ) 
        return;

    NSString *defaultPathString = [NSString stringWithUTF8String: defaultPath];
    NSURL *url = [NSURL fileURLWithPath:defaultPathString isDirectory:YES];
    [dialog setDirectoryURL:url];    
}


/* fixme: pathset should be pathSet */
static Result AllocPathSet( NSArray *urls, nfdpathset_t *pathset ) {
    assert(pathset);
    assert([urls count]);

    pathset->count = (size_t)[urls count];
    pathset->indices = NFDi_Malloc( sizeof(size_t)*pathset->count );
    if ( !pathset->indices ) {
        return NFD_ERROR;
    }

    // count the total space needed for buf
    size_t bufsize = 0;
    for ( NSURL *url in urls ) {
        NSString *path = [url path];
        bufsize += [path lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
    }

    pathset->buf = NFDi_Malloc( sizeof(char) * bufsize );
    if ( !pathset->buf ) {
        return NFD_ERROR;
    }

    // fill buf
    char *p_buf = pathset->buf;
    size_t count = 0;
    for ( NSURL *url in urls ) {
        NSString *path = [url path];
        const char *utf8Path = [path UTF8String];
        size_t byteLen = [path lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        memcpy( p_buf, utf8Path, byteLen );

        ptrdiff_t index = p_buf - pathset->buf;
        assert( index >= 0 );
        pathset->indices[count] = (size_t)index;

        p_buf += byteLen;
        ++count;
    }

    return NFD_OKAY;
}

/* public */


Result NFD::OpenDialog( const char *filterList, const char *defaultPath, std::string& outPath ) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];    
    NSOpenPanel *dialog = [NSOpenPanel openPanel];
    [dialog setAllowsMultipleSelection:NO];

    // Build the filter list
    AddFilterListToDialog(dialog, filterList);

    // Set the starting directory
    SetDefaultPath(dialog, defaultPath);

    Result nfdResult = NFD_CANCEL;
    if ( [dialog runModal] == NSModalResponseOK ) {
        NSURL *url = [dialog URL];
        outPath = [[url path] UTF8String];

        nfdResult = NFD_OKAY;
    }
    [pool release];

    [keyWindow makeKeyAndOrderFront:nil];
    return nfdResult;
}

Result NFD::OpenDialogMultiple(const char *filterList, const char *defaultPath, std::vector<std::string>& outPathsoutPaths) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
    
    NSOpenPanel *dialog = [NSOpenPanel openPanel];
    [dialog setAllowsMultipleSelection:YES];

    // Build the fiter list.
    AddFilterListToDialog(dialog, filterList);

    // Set the starting directory
    SetDefaultPath(dialog, defaultPath);
    
    Result nfdResult = NFD_CANCEL;
    if ( [dialog runModal] == NSModalResponseOK ) {
        NSArray *urls = [dialog URLs];

        if ( [urls count] == 0 ) {
            [pool release];
            [keyWindow makeKeyAndOrderFront:nil];            
            return NFD_CANCEL;
        }

        if ( AllocPathSet( urls, outPaths ) == NFD_ERROR ) {
            [pool release];
            [keyWindow makeKeyAndOrderFront:nil];            
            return NFD_ERROR;
        }

        nfdResult = NFD_OKAY;
    }
    [pool release];

    [keyWindow makeKeyAndOrderFront:nil];    
    return nfdResult;
}

Result NFD_SaveDialog( const char *filterList, const char *defaultPath, std::string& outPath ) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
    
    NSSavePanel *dialog = [NSSavePanel savePanel];
    [dialog setExtensionHidden:NO];
    
    // Build the filter list.
    AddFilterListToDialog(dialog, filterList);

    // Set the starting directory
    SetDefaultPath(dialog, defaultPath);

    Result nfdResult = NFD_CANCEL;
    if ( [dialog runModal] == NSModalResponseOK ) {
        NSURL *url = [dialog URL];
        outPath = [[url path] UTF8String];
        nfdResult = NFD_OKAY;
    }

    [pool release];
    [keyWindow makeKeyAndOrderFront:nil];
    return nfdResult;
}

Result NFD::PickFolder(const char *defaultPath, std::string& outPath) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSWindow *keyWindow = [[NSApplication sharedApplication] keyWindow];
    NSOpenPanel *dialog = [NSOpenPanel openPanel];
    [dialog setAllowsMultipleSelection:NO];
    [dialog setCanChooseDirectories:YES];
    [dialog setCanCreateDirectories:YES];
    [dialog setCanChooseFiles:NO];

    // Set the starting directory
    SetDefaultPath(dialog, defaultPath);

    Result nfdResult = NFD_CANCEL;
    if ( [dialog runModal] == NSModalResponseOK ) {
        NSURL *url = [dialog URL];
        outPath = [[url path] UTF8String];
        nfdResult = NFD_OKAY;
    }
    [pool release];

    [keyWindow makeKeyAndOrderFront:nil];
    return nfdResult;
}

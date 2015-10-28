/*  HBSubtitlesDefaultsController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesDefaultsController.h"
#import "HBSubtitlesDefaults.h"
#import "HBLanguagesSelection.h"

static void *HBSubtitlesDefaultsContext = &HBSubtitlesDefaultsContext;

@interface HBSubtitlesDefaultsController ()

@property (nonatomic, readonly) HBSubtitlesDefaults *settings;

@property (nonatomic, readonly) HBLanguagesSelection *languagesList;
@property (unsafe_unretained) IBOutlet HBLanguageArrayController *tableController;
@property (unsafe_unretained) IBOutlet NSButton *showAllButton;

@end

@implementation HBSubtitlesDefaultsController

- (instancetype)initWithSettings:(HBSubtitlesDefaults *)settings
{
    self = [super initWithWindowNibName:@"SubtitlesDefaults"];
    if (self)
    {
        _settings = settings;
        _languagesList = [[HBLanguagesSelection alloc] initWithLanguages:_settings.trackSelectionLanguages];
        _settings.undo = self.window.undoManager;
    }
    return self;
}

- (void)windowDidLoad
{
    [self addObserver:self forKeyPath:@"tableController.showSelectedOnly" options:0 context:HBSubtitlesDefaultsContext];

    if (self.settings.trackSelectionLanguages.count)
    {
        self.tableController.showSelectedOnly = YES;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBSubtitlesDefaultsContext)
    {
        if ([keyPath isEqualToString:@"tableController.showSelectedOnly"])
        {
            [self.showAllButton setState:!self.tableController.showSelectedOnly];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (IBAction)edit:(id)sender
{
    self.tableController.showSelectedOnly = !self.tableController.showSelectedOnly;
}

- (IBAction)done:(id)sender
{
    [self.window orderOut:nil];
    if (self.window.undoManager.canUndo)
    {
        self.settings.trackSelectionLanguages = [self.languagesList.selectedLanguages mutableCopy];
    }
    [NSApp endSheet:self.window returnCode:self.window.undoManager.canUndo];
}

- (void)dealloc
{
    @try {
        [self removeObserver:self forKeyPath:@"tableController.showSelectedOnly" context:HBSubtitlesDefaultsContext];
    } @catch (NSException * __unused exception) {}
}

@end

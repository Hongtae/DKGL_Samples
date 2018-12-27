//
//  AppDelegate.m
//  CocoaTest
//
//  Created by tiff on 11/17/16.
//  Copyright © 2016 icondb. All rights reserved.
//

#import "AppDelegate.h"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application
	NSLog(@"%s", __PRETTY_FUNCTION__);
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
	NSLog(@"%s", __PRETTY_FUNCTION__);
}

- (instancetype)init
{
	NSLog(@"%s", __PRETTY_FUNCTION__);
	self = [super init];
	return self;
}

- (void)dealloc
{
	NSLog(@"%s", __PRETTY_FUNCTION__);
}

@end

//
//  LcdPanel.h
//  CocoaWget
//
//  Created by kikeland on 27/06/10.
//  Copyright 2010 ebjsoft. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface LcdPanel : NSView {
	NSColor *bgColor;
	NSColor *bg2Color;
	NSColor *linebgColor;
	NSString *upperText;
	NSString *lowerText;
	NSImage *image;
	NSImage *imageMonkey;
	float progress;
}

- (void)drawText:(NSString *)text;
- (void)setUpperText:(NSString *)uText;
- (void)setLowerText:(NSString *)lText;
- (void)drawImage:(NSImage *)aImage;
- (void)setProgress:(float)aProgress;
- (void)clear;

@end

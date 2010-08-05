//
//  LcdPanel.m
//  CocoaWget
//
//  Created by kikeland on 27/06/10.
//  Copyright 2010 ebjsoft. All rights reserved.
//

#import "LcdPanel.h"

static LcdPanel *k_lcdPanel = nil;
static const NSString *k_Text = @"";

@implementation LcdPanel

#pragma mark Mouse Delegate
- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent {
    // determine if I handle theEvent
    NSPoint coordinates = [theEvent locationInWindow];
    NSPoint relativeCoord = [self convertPoint:coordinates fromView:nil];
    NSLog(@"Mouse pressed at %f, %f", relativeCoord.x, relativeCoord.y);
    // if not...
    [super mouseDown:theEvent];
}

#pragma mark -
#pragma mark Initialization
- (id)initWithFrame:(NSRect)frame {
	NSLog(@"LCD Panel initialization"); 
    self = [super initWithFrame:frame];
    if (self) {
		bgColor = [[NSColor colorWithCalibratedRed:(239/255.0) green:(242/255.0) blue:(230/255.0) alpha:1.0] retain];
		bg2Color = [[NSColor colorWithCalibratedRed:(235/255.0) green:(239/255.0) blue:(218/255.0) alpha:1.0] retain];
		linebgColor = [[NSColor colorWithCalibratedRed:(125/255.0) green:(131/255.0) blue:(124/255.0) alpha:1.0] retain];
		upperText = nil;
		lowerText = nil;
		image = nil;
		k_lcdPanel = self;
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
	// Draw lcd Panel
	
	/*NSImage *backGround = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"apple" 
																					 ofType:@"tiff" 
																				inDirectory:@""]];
	NSRect srcRect = {{0,0}, [backGround size]};
	[backGround drawInRect:dirtyRect 
				  fromRect:srcRect 
				 operation:NSCompositeCopy 
				  fraction:1];*/
		
	
	// First background color
	float radius;
	NSRect textRect = NSMakeRect(0, 0, dirtyRect.size.width, dirtyRect.size.height);
	radius = textRect.size.height / 8;
	[bg2Color set];
	NSBezierPath *path;
	path = [NSBezierPath bezierPathWithRoundedRect:textRect xRadius:radius yRadius:radius];
	[path fill];
	
	// Second background color
	textRect = NSMakeRect(0, dirtyRect.size.height * 0.5, dirtyRect.size.width, dirtyRect.size.height * 0.5);
	[bgColor set];
	path = [NSBezierPath bezierPathWithRoundedRect:textRect xRadius:radius yRadius:radius];
	[path fill];
	
	// Draw the border line
	textRect = NSMakeRect(0, 0, dirtyRect.size.width, dirtyRect.size.height);
	radius = textRect.size.height / 8;
	path = [NSBezierPath bezierPathWithRoundedRect:textRect xRadius:radius yRadius:radius];
	[linebgColor setStroke];
	[path setLineWidth:2];
	[path stroke];
	
	// Draw the image
	if (image) {
		NSRect imageRect = NSMakeRect(0, 0, image.size.width, image.size.height);
		NSRect destRect = NSMakeRect((dirtyRect.size.width / 2) - 16, 4, dirtyRect.size.height - 8, dirtyRect.size.height - 8);
		[image drawInRect:destRect 
				 fromRect:imageRect 
				operation:NSCompositeXOR 
				 fraction:1];
	}
	
	// Draw the upperText
	if ([upperText length] > 0) {
		textRect = NSMakeRect(0, dirtyRect.size.height * 0.5, dirtyRect.size.width, dirtyRect.size.height * 0.5);
		NSString* text = [NSString stringWithFormat:@"%@", upperText];
		NSFont* font = [NSFont fontWithName:@"Chalkboard-bold" size:textRect.size.height/2.2];
		NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys: 
									font, NSFontAttributeName,
									[NSColor blackColor], NSForegroundColorAttributeName,
									nil];
	
		NSSize size = [text sizeWithAttributes:attributes];
		float offset = (textRect.size.width - size.width) / 2;
		NSPoint textPoint = NSMakePoint(textRect.origin.x + offset, textRect.origin.y * 1.2);
		[text drawAtPoint:textPoint withAttributes:attributes];
	}
	
	// Draw the lowerText
	if ([lowerText length] > 0) {
		textRect = NSMakeRect(0, dirtyRect.size.height * 0.5, dirtyRect.size.width, dirtyRect.size.height * 0.5);
		NSString* text = [NSString stringWithFormat:@"%@", lowerText];
		NSFont* font = [NSFont fontWithName:@"Chalkboard" size:textRect.size.height/2.2];
		NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys: 
									font, NSFontAttributeName,
									[NSColor blackColor], NSForegroundColorAttributeName,
									nil];
		
		NSSize size = [text sizeWithAttributes:attributes];
		float offset = (textRect.size.width - size.width) / 2;
		NSPoint textPoint = NSMakePoint(textRect.origin.x + offset, textRect.origin.y * 0.7);
		[text drawAtPoint:textPoint withAttributes:attributes];
	}
	
	// Draw the progressBar
	if (progress > 0) {
		const NSRect k_progressBarRect = NSMakeRect(0.05, 0.1, 0.9, 0.2);
		[[NSGraphicsContext currentContext] saveGraphicsState];
		NSRect progressBarRect = NSMakeRect(dirtyRect.size.width * k_progressBarRect.origin.x, 
											dirtyRect.size.height * k_progressBarRect.origin.y, 
											dirtyRect.size.width * k_progressBarRect.size.width, 
											dirtyRect.size.height * k_progressBarRect.size.height);
		radius = progressBarRect.size.height / 2;
		path = [NSBezierPath bezierPathWithRoundedRect:progressBarRect 
														 xRadius:radius 
														 yRadius:radius];
		[path setClip];
		[[NSColor whiteColor] setFill];
		[[NSBezierPath bezierPathWithRect:progressBarRect] fill];
		progressBarRect.size.width *= progress;
		[[NSColor colorWithDeviceRed:(82/255.0) green:(89/255.0) blue:(72/255.0) alpha:1] setFill];
		[[NSBezierPath bezierPathWithRect:progressBarRect] fill];
		[[NSGraphicsContext currentContext] restoreGraphicsState];
		[[NSColor blackColor] setStroke];
		[path setLineWidth:1];
		[path stroke];
	}
}

#pragma mark -
#pragma mark Display routines
- (void)drawText:(NSString *)text {
	k_Text = text;
	[self setNeedsDisplay:YES];
}

- (void)setUpperText:(NSString *)uText {
	image = nil;
	[uText retain];
	[upperText release];
	upperText = uText;
	[self setNeedsDisplay:YES];
}

- (void)setLowerText:(NSString *)lText {
	image = nil;
	[lText retain];
	[lowerText release];
	lowerText = lText;
	[self setNeedsDisplay:YES];
}

- (void)drawImage:(NSImage *)aImage {
	[aImage retain];
	[image release];
	image = aImage;
	[self setNeedsDisplay:YES];
}

- (void)setProgress:(float)aProgress {
	progress = aProgress;
	[self setNeedsDisplay:YES];
}

- (void)clear {
	upperText = @"";
	lowerText = @"";
	imageMonkey = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForImageResource:@"monkey.png"]];
	image = imageMonkey;
	[self setProgress:0.0];
	[self setNeedsDisplay:YES];
}

#pragma mark -
- (void)dealloc {
	[bgColor release];
	[bg2Color release];
	[upperText release];
	[lowerText release];
	[image release];
	[imageMonkey release];
	
	[super dealloc];
}

@end

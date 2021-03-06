//
//  SyncClient.h
//  SampleCollection
//
//  Created by niustock on 16/1/29.
//  Copyright © 2016年 caoym. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface SyncClient : NSObject
- (instancetype)initWithLocal:(NSString*)local forType:(NSString*)type withHost:(NSString*)host withRemote:(NSString*)remote;
-(void) update;
-(void) commit;
-(void) delFile:(NSString*)name;

@end

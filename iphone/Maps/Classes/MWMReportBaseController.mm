#import "MWMAuthorizationCommon.h"
#import "MWMReportBaseController.h"

#include "indexer/osm_editor.hpp"

@implementation MWMReportBaseController
{
  m2::PointD m_point;
}

- (void)configNavBar
{
  self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:L(@"editor_report_problem_send_button")
                                            style:UIBarButtonItemStylePlain target:self action:@selector(send)];
}

- (void)send
{
  [self doesNotRecognizeSelector:_cmd];
}

- (void)sendNote:(string const &)note
{
  osm::Editor::Instance().CreateNote(m_point, note);
  [self.navigationController popToRootViewControllerAnimated:YES];
}

- (void)setPoint:(m2::PointD const &)point
{
  m_point = point;
}

- (m2::PointD const &)point
{
  return m_point;
}

@end

Feature: Forum listing page
  To have people accessing community content
  As any user
  I should see an empty listing page

  Scenario: Forum listing page exists
    When I go to "/forum"
    Then I should see "Forum"
    And I should get a "200" HTTP response
